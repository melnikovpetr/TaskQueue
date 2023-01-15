#include "TestDialog.h"
#include "ui_TestDialog.h"

#include <TaskLauncher.h>

#include <algorithm>
#include <cmath>
#include <sstream>

namespace bind_ph = std::placeholders;

using Array = std::vector<ArrayValue>;
using ThreadId = std::thread::id;

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
#include <unistd.h>
static size_t availableSystemMemory()
{
  return ::sysconf(_SC_PAGE_SIZE) * ::sysconf(_SC_AVPHYS_PAGES);
}
#endif

static size_t operCount(size_t arraySize)
{
  assert(arraySize > 0);
  return arraySize * std::log(arraySize);
}

static size_t minArraySize(ThreadCount threadCount)
{
  assert(threadCount > 0);
  return threadCount * 2;
}

static std::string threadIdToStr(ThreadId threadId)
{
  std::ostringstream ss{};
  ss << threadId;
  return ss.str();
}

static size_t maxArraySize()
{
  return availableSystemMemory() / (sizeof(ArrayValue) * 2);
}

static std::vector<std::pair<size_t, size_t>> arrayThreadRanges(ThreadCount threadCount, size_t arraySize)
{
  assert((threadCount > 0) && (arraySize >= minArraySize(threadCount)));
  decltype(arrayThreadRanges(threadCount, arraySize)) result(threadCount);
  size_t arrayThreadRangeSize = arraySize / threadCount;
  for (size_t threadIndex = 0; threadIndex < (threadCount - 1); ++threadIndex)
  {
    result[threadIndex].first = threadIndex * arrayThreadRangeSize;
    result[threadIndex].second =
      threadIndex < (threadCount - 1) ? result[threadIndex].first + arrayThreadRangeSize : arraySize;
  }
  return result;
}

static size_t progressGrain(size_t operCount)
{
  return operCount >= 20 ? operCount / 20 : 1;
}

class TaskEvent : public QEvent
{
public:
  static int taskEndEventType()
  {
    static auto type = QEvent::registerEventType();
    return type;
  }

  static int taskProgressEventType()
  {
    static auto type = QEvent::registerEventType();
    return type;
  }

  static void taskEndEventFn(QObject* reseiver, TaskId taskId, TaskReturn&& taskReturn)
  {
    QCoreApplication::postEvent(
      reseiver, new TaskEvent{ taskEndEventType(), taskId, std::any_cast<QVariant>(taskReturn) });
  }

  static void taskStartEventFn(QObject* reseiver, TaskId taskId, const ThreadId threadId, size_t from, size_t to)
  {
    QVariant thrdId = QString::fromStdString(::threadIdToStr(threadId));
    QVariant taskRange = "[" + QString::number(from) + ", " + QString::number(to) + ")";
    QCoreApplication::postEvent(
      reseiver, new TaskEvent{ taskEndEventType(), taskId, QVariantList{ thrdId, taskRange } });
  }

  static void taskProgressEventFn(QObject* reseiver, TaskId taskId, double taskProgress)
  {
    QCoreApplication::postEvent(reseiver, new TaskEvent{ taskEndEventType(), taskId, taskProgress });
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
    , _taskLauncher{ std::bind(TaskEvent::taskEndEventFn, self, bind_ph::_1, bind_ph::_2) }
    , _array(::minArraySize(_taskLauncher.threadCount()))
    , _interruptFlag{ false }
  {
    _taskLauncher.start();
  }

  void sort()
  {
    _interruptFlag = false;
    _taskLauncher.queueBatch(0, _array.size(), 0, [this](TaskId taskId, size_t from, size_t to) {
      auto threadOperIndex = size_t(0);
      auto threadOperCount = ::operCount(to - from);
      auto progressGrain = ::progressGrain(threadOperCount);
      auto threadCmpPred = [this, taskId, threadOperIndex, threadOperCount, progressGrain](
                             const ArrayValue& l, const ArrayValue& r) mutable -> bool {
        if (!(threadOperIndex % progressGrain))
        {
          TaskEvent::taskProgressEventFn(_self, taskId, static_cast<double>(threadOperIndex) / threadOperCount);
          if (_interruptFlag)
            throw std::runtime_error("Interrupted!");
        }
        threadOperIndex++;
        return l < r;
      };
      TaskEvent::taskStartEventFn(_self, taskId, std::this_thread::get_id(), from, to);
      std::sort(_array.begin() + from, _array.begin() + to, threadCmpPred);
    });
  }

  void interrupt()
  {
    _taskLauncher.clear();
    _interruptFlag = true;
  }

  ThreadCount threadCount() const { return _taskLauncher.threadCount(); }

  size_t arraySize() const { return _array.size(); }

  size_t generateArray() const { return _array.size(); }

private:
  TestDialog* _self;
  TaskLauncher _taskLauncher;
  Array _array;
  bool _interruptFlag;
};

TestDialog::TestDialog(QWidget* parent)
  : QDialog(parent)
  , _ui(new Ui::TestDialog)
  , _data(new TestDialogInternal{ this })
{
  _ui->setupUi(this);

  connect(_ui->arraySizeSldr, &QSlider::valueChanged, this, &TestDialog::changeArraySize);
  connect(_ui->applyArraySizeBttn, &QPushButton::clicked, this, &TestDialog::applyArraySize);
  connect(_ui->startStopBttn, &QPushButton::toggled, this, &TestDialog::startStopSorting);

  _ui->arraySizeSldr->setMinimum(::minArraySize(_data->threadCount()));
  _ui->arraySizeSldr->setMaximum(::maxArraySize());
  _ui->arraySizeSldr->setValue(_data->arraySize());
}

TestDialog::~TestDialog()
{
  delete _ui;
  delete _data;
}

bool TestDialog::event(QEvent* event)
{
  if (event->type() == TaskEvent::taskEndEventType())
  {
    auto taskEvent = static_cast<TaskEvent*>(event);
    return true;
  }
  else if (event->type() == TaskEvent::taskProgressEventType())
  {
    auto taskEvent = static_cast<TaskEvent*>(event);
    return true;
  }
  return QDialog::event(event);
}

void TestDialog::changeArraySize()
{
  _ui->arraySizeEdit->setText(QString::number(_ui->arraySizeSldr->value()));
}

void TestDialog::resetArraySize() {}

void TestDialog::applyArraySize() {}

void TestDialog::startStopSorting(bool start)
{
  if (start)
    _data->sort();
  else
    _data->interrupt();
}
