#include "TestDialog.h"
#include "ui_TestDialog.h"

#include <TaskLauncher.h>

#include <QtGui/QScreen>
#include <QtGui/QStandardItemModel>

#include <QtWidgets/QMessageBox>
#include <QtWidgets/QStatusBar>

#include <algorithm>
#include <cmath>
#include <random>
#include <sstream>

namespace bind_ph = std::placeholders;

using Array = std::vector<ArrayValue>;
using ThreadId = std::thread::id;

Q_DECLARE_METATYPE(TaskResult<QVariant>)

#define PROP_MIN_ARR_SZ "MIN_ARRAY_SIZE"
#define PROP_MAX_ARR_SZ "MAX_ARRAY_SIZE"
#define PROP_ARR_SZ "ARRAY_SIZE"

#ifdef WIN32
#include <windows.h>
static size_t availableSystemMemory()
{
  MEMORYSTATUSEX statex;
  statex.dwLength = sizeof(statex);
  ::GlobalMemoryStatusEx(&statex);
  return statex.ullAvailPhys;
}
#else
//#include <unistd.h>
// static size_t availableSystemMemory()
//{
//  return ::sysconf(_SC_PAGE_SIZE) * ::sysconf(_SC_AVPHYS_PAGES);
//}
#include <fstream>
static size_t availableSystemMemory()
{
  std::string title{};
  size_t value{ 0 };
  std::string units{};
  std::ifstream file{ "/proc/meminfo" };
  while (file >> title)
  {
    if (title == "MemAvailable:")
      return (file >> value) ? ((file >> units) ? (units == "kB" ? value * 1024 : 0) : 0) : 0;
    // Ignore the rest of the line
    file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }
  return 0; // Nothing found
}
#endif

static size_t operCount(size_t arraySize)
{
  assert(arraySize > 0);
  return 2 * arraySize * std::log(arraySize);
}

static size_t minArraySize(ThreadCount threadCount)
{
  assert(threadCount > 0);
  return threadCount * 2;
}

static size_t maxArraySize()
{
  return 0.8 * availableSystemMemory() / sizeof(ArrayValue); // 80% of available memory
}

template <typename T1, typename T2>
static T2 scaleToAnotherRange(T1 r1Min, T1 r1Max, T1 r1Value, T2 r2Min, T2 r2Max)
{
  return r2Min + (r2Max - r2Min) * (static_cast<double>(r1Value - r1Min) / (r1Max - r1Min));
}

static std::string threadIdToStr(ThreadId threadId)
{
  std::ostringstream ss{};
  ss << threadId;
  return ss.str();
}

static size_t progressGrain(size_t operCount)
{
  return operCount >= 20 ? operCount / 20 : 1;
}

class TaskEvent : public QEvent
{
public:
  static int taskStartEventType()
  {
    static auto type = QEvent::registerEventType();
    return type;
  }

  static int taskProgressEventType()
  {
    static auto type = QEvent::registerEventType();
    return type;
  }

  static int taskEndEventType()
  {
    static auto type = QEvent::registerEventType();
    return type;
  }

  static void taskStartEventFn(QObject* reseiver, TaskId taskId, const ThreadId threadId, size_t from, size_t to)
  {
    QVariant thrdId = QString::fromStdString(::threadIdToStr(threadId));
    QVariant taskRange = "[" + QString::number(from) + ", " + QString::number(to) + ")";
    QCoreApplication::postEvent(reseiver, new TaskEvent{ taskStartEventType(), taskId, QVariantList{ thrdId, taskRange } });
  }

  static void taskProgressEventFn(QObject* reseiver, TaskId taskId, double taskProgress)
  {
    QCoreApplication::postEvent(reseiver, new TaskEvent{ taskProgressEventType(), taskId, taskProgress });
  }

  static void taskEndEventFn(QObject* reseiver, TaskId taskId, const TaskResult<QVariant>& taskResult)
  {
    QCoreApplication::postEvent(reseiver, new TaskEvent{ taskEndEventType(), taskId, QVariant::fromValue(taskResult) });
  }

public:
  TaskEvent(int type, TaskId taskId, const QVariant& taskInfo)
    : QEvent(static_cast<QEvent::Type>(type))
    , _taskId{ taskId }
    , _taskInfo{ taskInfo }
  {
  }

  TaskId taskId() const { return _taskId; }
  const QVariant& taskInfo() const { return _taskInfo; }

private:
  TaskId _taskId;
  QVariant _taskInfo;
};

class TestDialogInternal
{
public:
  TestDialogInternal(TestDialog* self)
    : _self{ self }
    , _taskLauncher{}
    , _array(::minArraySize(_taskLauncher.threadCount()))
    , _interruptFlag{ false }
  {
    generateArray(_array.size());
  }

  ~TestDialogInternal() { _taskLauncher.stopAndWait(&_interruptFlag); }

  auto threadCount() const { return _taskLauncher.threadCount(); }

  auto arraySize() const { return _array.size(); }

  auto saveTask(TaskId taskId) { return _taskIndexes.insert({ taskId, { _taskIndexes.size(), false } }).first->second.first; }

  auto clearTasks() { _taskIndexes.clear(); }

  auto finishTask(TaskId taskId)
  {
    auto& taskIndex = _taskIndexes.at(taskId);
    taskIndex.second = true;
    return taskIndex.first;
  }

  auto taskCount() const { return _taskIndexes.size(); }

  auto isTaskSaved(TaskId taskId) const { return _taskIndexes.find(taskId) != _taskIndexes.cend(); }

  auto taskIndex(TaskId taskId) const { return _taskIndexes.at(taskId).first; }

  auto isTaskFinished(TaskId taskId) const { return _taskIndexes.at(taskId).second; }

  auto areAllTasksFinished() const
  {
    auto result = taskCount() == threadCount();
    for (const auto& taskIndex : _taskIndexes)
      if (!(result = result && taskIndex.second.second))
        break;
    return result;
  }

  void sort()
  {
    TaskEndEventFn<QVariant> taskEndEventFn = std::bind(TaskEvent::taskEndEventFn, _self, bind_ph::_1, bind_ph::_2);
    clearTasks();
    _taskLauncher.queueBatch(
      0, _array.size(), 0,
      [this](TaskId taskId, size_t from, size_t to) -> QVariant
      {
        auto threadOperIndex = size_t(0);
        auto threadOperCount = ::operCount(to - from);
        auto progressGrain = ::progressGrain(threadOperCount);
        auto threadCmpPred = [this, taskId, &threadOperIndex, threadOperCount, progressGrain](const ArrayValue& l, const ArrayValue& r) mutable -> bool
        {
          if (threadOperIndex && !(threadOperIndex % progressGrain))
          {
            auto progress = static_cast<double>(threadOperIndex) / threadOperCount;
            TaskEvent::taskProgressEventFn(_self, taskId, progress >= 0.99 ? 0.99 : progress);
            if (_interruptFlag)
              throw std::runtime_error("Task with id = " + std::to_string(taskId) + " was interrupted!");
          }
          threadOperIndex++;
          return l < r;
        };
        TaskEvent::taskStartEventFn(_self, taskId, std::this_thread::get_id(), from, to);
        std::sort(_array.begin() + from, _array.begin() + to, threadCmpPred);
        return QString("min = %1, max = %2").arg(_array[from]).arg(_array[to - 1]);
      },
      taskEndEventFn);
  }

  void interrupt()
  {
    _taskLauncher.stopAndWait(&_interruptFlag);
    _taskLauncher.clear();
    _interruptFlag = false;
    _taskLauncher.start();
  }

  void generateArray(size_t arraySize)
  {
    interrupt();
    decltype(_array){}.swap(_array);
    _array.resize(arraySize);
    auto taskHandles = _taskLauncher.queueBatch(0, _array.size(), 0,
      [this](TaskId taskId, size_t from, size_t to)
      {
        std::uniform_int_distribution<ArrayValue> distribution(0, (std::numeric_limits<ArrayValue>::max)());
        std::mt19937 randomEngine{ std::random_device{}() };
        auto random = std::bind(distribution, randomEngine);
        for (auto index = from; index < to; ++index)
          _array[index] = random();
      });
    for (auto& taskHandle : taskHandles)
      taskHandle.result.wait();
  }

private:
  TestDialog* _self;
  TaskLauncher _taskLauncher;
  Array _array;
  std::atomic<bool> _interruptFlag;
  std::unordered_map<TaskId, std::pair<size_t, bool>> _taskIndexes;
};

// clang-format off
struct TaskStatusCols { enum : int { ID, THREAD_ID, INFO, PROGRESS, RESULT, _COUNT }; };
// clang-format on

TestDialog::TestDialog(QWidget* parent)
  : QDialog(parent)
  , _ui(new Ui::TestDialog)
  , _data(new TestDialogInternal{ this })
{
  _ui->setupUi(this);

  {
    auto statusBar = new QStatusBar{ this };
    layout()->addWidget(statusBar);
    statusBar->showMessage(QString("Thread pool size: %1").arg(_data->threadCount()));
  }
  {
    auto taskStatusModel = new QStandardItemModel{ this };

    taskStatusModel->setColumnCount(TaskStatusCols::_COUNT);
    taskStatusModel->setHeaderData(TaskStatusCols::ID, Qt::Horizontal, "Id");
    taskStatusModel->setHeaderData(TaskStatusCols::THREAD_ID, Qt::Horizontal, "Thread Id");
    taskStatusModel->setHeaderData(TaskStatusCols::INFO, Qt::Horizontal, "Array Index Range");
    taskStatusModel->setHeaderData(TaskStatusCols::PROGRESS, Qt::Horizontal, "Progress");
    taskStatusModel->setHeaderData(TaskStatusCols::RESULT, Qt::Horizontal, "Result");

    _ui->taskStatusTable->setModel(taskStatusModel);
  }

  auto minArraySize = ::minArraySize(_data->threadCount());
  _ui->arraySizeSldr->setProperty(PROP_MIN_ARR_SZ, QVariant::fromValue(minArraySize));
  _ui->arraySizeSldr->setMinimum(0);

  auto maxArraySize = ::maxArraySize();
  _ui->arraySizeSldr->setProperty(PROP_MAX_ARR_SZ, maxArraySize ? QVariant::fromValue(maxArraySize) : _ui->arraySizeSldr->property(PROP_MIN_ARR_SZ));
  _ui->arraySizeSldr->setMaximum(maxArraySize ? QGuiApplication::primaryScreen()->size().width() : _ui->arraySizeSldr->minimum());

  if (!maxArraySize)
    QMessageBox::warning(nullptr, "Warning!", "Can't determine maximum array size!");

  _ui->arraySizeSldr->setValue(
    ::scaleToAnotherRange(minArraySize, maxArraySize, _data->arraySize(), _ui->arraySizeSldr->minimum(), _ui->arraySizeSldr->maximum()));

  _ui->arraySizeEdit->setProperty(PROP_ARR_SZ, QVariant::fromValue(_data->arraySize()));
  _ui->arraySizeEdit->setText(_ui->arraySizeEdit->property(PROP_ARR_SZ).toString());

  connect(_ui->arraySizeSldr, &QSlider::valueChanged, this, &TestDialog::changeArraySize);
  connect(_ui->applyArraySizeBttn, &QPushButton::clicked, this, &TestDialog::applyArraySize);
  connect(_ui->startStopBttn, &QPushButton::toggled, this, &TestDialog::startStopSorting);
}

TestDialog::~TestDialog()
{
  delete _ui;
  delete _data;
}

bool TestDialog::event(QEvent* event)
{
  if (event->type() == TaskEvent::taskStartEventType())
  {
    auto taskEvent = static_cast<TaskEvent*>(event);
    auto taskId = taskEvent->taskId();
    auto taskInfo = taskEvent->taskInfo().toList();
    auto threadId = taskInfo.size() ? taskInfo.front() : QVariant{};
    auto taskRange = taskInfo.size() ? taskInfo.back() : QVariant{};

    auto taskStatusModel = _ui->taskStatusTable->model();
    auto row = _data->saveTask(taskId);

    taskStatusModel->insertRow(row);
    taskStatusModel->setData(taskStatusModel->index(row, TaskStatusCols::ID), taskId);
    taskStatusModel->setData(taskStatusModel->index(row, TaskStatusCols::THREAD_ID), threadId);
    taskStatusModel->setData(taskStatusModel->index(row, TaskStatusCols::INFO), taskRange);
    taskStatusModel->setData(taskStatusModel->index(row, TaskStatusCols::PROGRESS), 0.0);
    taskStatusModel->setData(taskStatusModel->index(row, TaskStatusCols::RESULT), "");

    _ui->taskStatusTable->resizeColumnsToContents();

    return true;
  }
  else if (event->type() == TaskEvent::taskProgressEventType())
  {
    auto taskEvent = static_cast<TaskEvent*>(event);
    auto taskId = taskEvent->taskId();
    if (_data->isTaskSaved(taskId))
    {
      auto taskStatusModel = _ui->taskStatusTable->model();
      auto row = _data->taskIndex(taskId);
      if (row < taskStatusModel->rowCount())
      {
        taskStatusModel->setData(taskStatusModel->index(row, TaskStatusCols::PROGRESS), taskEvent->taskInfo());

        _ui->taskStatusTable->resizeColumnsToContents();
      }
    }

    return true;
  }
  else if (event->type() == TaskEvent::taskEndEventType())
  {
    auto taskEvent = static_cast<TaskEvent*>(event);
    auto taskId = taskEvent->taskId();

    if (_data->isTaskSaved(taskId))
    {
      auto taskStatusModel = _ui->taskStatusTable->model();
      auto row = _data->finishTask(taskId);
      if (row < taskStatusModel->rowCount())
      {
        try
        {
          auto taskResult = taskEvent->taskInfo().value<TaskResult<QVariant>>().get();
          taskStatusModel->setData(taskStatusModel->index(row, TaskStatusCols::PROGRESS), 1.0);
          taskStatusModel->setData(taskStatusModel->index(row, TaskStatusCols::RESULT), taskResult);
        }
        catch (const std::runtime_error& e)
        {
          taskStatusModel->setData(taskStatusModel->index(row, TaskStatusCols::RESULT), e.what());
        }

        _ui->taskStatusTable->resizeColumnsToContents();
      }
      if (!_data->areAllTasksFinished())
      {
        QSignalBlocker signalBlocker{ _ui->startStopBttn };
        _ui->startStopBttn->setChecked(false);
      }
    }

    return true;
  }

  return QDialog::event(event);
}

void TestDialog::changeArraySize()
{
  _ui->arraySizeEdit->setProperty(PROP_ARR_SZ,
    QVariant::fromValue(::scaleToAnotherRange(_ui->arraySizeSldr->minimum(), _ui->arraySizeSldr->maximum(), _ui->arraySizeSldr->value(),
      _ui->arraySizeSldr->property(PROP_MIN_ARR_SZ).value<size_t>(), _ui->arraySizeSldr->property(PROP_MAX_ARR_SZ).value<size_t>())));
  _ui->arraySizeEdit->setText(_ui->arraySizeEdit->property(PROP_ARR_SZ).toString());
}

void TestDialog::applyArraySize()
{
  QMessageBox msgBox(QMessageBox::Warning, "Please wait! Array generation!", "", QMessageBox::NoButton, this);
  // msgBox.setModal(false);
  msgBox.show();
  QCoreApplication::processEvents();
  _data->generateArray(_ui->arraySizeEdit->property(PROP_ARR_SZ).value<size_t>());
}

void TestDialog::startStopSorting(bool start)
{
  if (start)
  {
    auto taskStatusModel = _ui->taskStatusTable->model();
    _data->interrupt();
    taskStatusModel->removeRows(0, taskStatusModel->rowCount());
    _data->sort();
  }
  else
    _data->interrupt();
}
