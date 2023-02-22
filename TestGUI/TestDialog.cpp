#include "TestDialog.h"
#include "ui_TestDialog.h"

#include <ArraySort.h>

#include <QtGui/QScreen>
#include <QtGui/QStandardItemModel>

#include <QtWidgets/QMessageBox>
#include <QtWidgets/QStatusBar>

using namespace std::placeholders;

Q_DECLARE_METATYPE(SortTaskResult)

#define PROP_MIN_ARR_SZ "MIN_ARRAY_SIZE"
#define PROP_MAX_ARR_SZ "MAX_ARRAY_SIZE"
#define PROP_ARR_SZ "ARRAY_SIZE"

template <typename T1, typename T2>
static T2 scaleToAnotherRange(T1 r1Min, T1 r1Max, T1 r1Value, T2 r2Min, T2 r2Max)
{
  return r2Min + (r2Max - r2Min) * (static_cast<double>(r1Value - r1Min) / (r1Max - r1Min));
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

  static void taskStartEventFn(QObject* reseiver, TaskId taskId, ThreadId threadId, size_t from, size_t to)
  {
    QVariant thrdId = QString::fromStdString(ArraySort::threadIdToStr(threadId));
    QVariant taskRange = "[" + QString::number(from) + ", " + QString::number(to) + ")";
    QCoreApplication::postEvent(reseiver, new TaskEvent{ taskStartEventType(), taskId, QVariantList{ thrdId, taskRange } });
  }

  static void taskProgressEventFn(QObject* reseiver, TaskId taskId, SortTaskProgress taskProgress)
  {
    QCoreApplication::postEvent(reseiver, new TaskEvent{ taskProgressEventType(), taskId, taskProgress });
  }

  static void taskEndEventFn(QObject* reseiver, TaskId taskId, const SortTaskResult& taskResult)
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

// clang-format off
struct TaskStatusCols { enum : int { ID, THREAD_ID, INFO, PROGRESS, RESULT, _COUNT }; };
// clang-format on

TestDialog::TestDialog(QWidget* parent)
  : QDialog(parent)
  , _ui(new Ui::TestDialog)
  , _data(new ArraySort{ std::bind(TaskEvent::taskStartEventFn, this, _1, _2, _3, _4), std::bind(TaskEvent::taskProgressEventFn, this, _1, _2),
      std::bind(TaskEvent::taskEndEventFn, this, _1, _2) })
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

  auto minArraySize = _data->minArraySize();
  _ui->arraySizeSldr->setProperty(PROP_MIN_ARR_SZ, QVariant::fromValue(minArraySize));
  _ui->arraySizeSldr->setMinimum(0);

  auto maxArraySize = ArraySort::maxArraySize();
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
          auto taskResult = taskEvent->taskInfo().value<SortTaskResult>().get();
          taskStatusModel->setData(taskStatusModel->index(row, TaskStatusCols::PROGRESS), 1.0);
          taskStatusModel->setData(taskStatusModel->index(row, TaskStatusCols::RESULT), QString::fromStdString(taskResult));
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
