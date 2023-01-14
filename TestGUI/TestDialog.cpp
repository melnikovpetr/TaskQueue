#include "TestDialog.h"
#include "ui_TestDialog.h"

#include <TaskLauncher.h>

namespace bind_ph = std::placeholders;

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

  static void taskProgressEventFn(QObject* reseiver, TaskId taskId, int taskProgress)
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
  using Array = std::vector<TestDialog::ArrayValue>;

  static size_t calcOperCount(size_t arraySize);
  static std::vector<Array> createArrayParts(ThreadCount threadCount, size_t arraySize);
  static size_t calcProgressGrain(size_t operCount);

public:
  TestDialogInternal(TestDialog* self)
    : taskLauncher{ std::bind(TaskEvent::taskEndEventFn, self, bind_ph::_1, bind_ph::_2) }
    , array(taskLauncher.threadCount() * 2)
    , arrayParts(taskLauncher.threadCount(), Array(array.size() / taskLauncher.threadCount()))
    , operCount{}
    , progressGrain{ operGrain >= 20 ? operGrain / 20 : 1 }
    , interraptFlag{ false }
  {
  }

  bool operator()(const TestDialog::ArrayValue& l, const TestDialog::ArrayValue& r)
  {
    if (!((pntIndex - begin) % progressGrain))
    {
      auto prevGlobalOperindex = globalOperIndex.fetch_add(progressGrain, std::memory_order_relaxed);
      if (!threadNumber)
        this->UpdateProgress(static_cast<double>(prevGlobalOperindex) / operCount);
      if (this->GetAbortExecute())
        break;
    }
  }

  TaskLauncher taskLauncher;
  Array array;
  std::vector<Array> arrayParts;
  size_t operCount;
  int progressGrain;
  bool interruptFlag;
};

TestDialog::TestDialog(QWidget* parent)
  : QDialog(parent)
  , _ui(new Ui::TestDialog)
  , _data(new TestDialogInternal{ this })
{
  _ui->setupUi(this);
  /*
  QVBoxLayout *verticalLayout_2;
  QTableView *taskStatusTable;
  QHBoxLayout *horizontalLayout;
  QHBoxLayout *horizontalLayout_2;
  QSlider *horizontalSlider;
  QLineEdit *arraySizeEdit;
  QPushButton *resetArraySizeBttn;
  QPushButton *applyArraySizeBttn;
  QPushButton *startStopBttn;*/

  _data->taskLauncher.start();
}

TestDialog::~TestDialog()
{
  delete _ui;
  delete _data;
}

bool TestDialog::event(QEvent* event)
{
  if (event->type() == TaskEvent::taskEndEventType())
    QMessageBox::information(nullptr, "Inform", static_cast<TaskEvent*>(event)->taskInfo().toString());
  return true;
}

void TestDialog::changeArraySize() {}

void TestDialog::resetArraySize() {}

void TestDialog::applyArraySize() {}

void TestDialog::startStopSorting()
{
  for (auto& num : { 1, 2, 4, 5, 6, 7, 8, 9 })
    launcher.queueTask(taskFn, "Task " + QString::number(num));
}

std::pair<TestDialog::ArrayValue, TestDialog::ArrayValue> TestDialog::operator()(size_t begin, size_t end) const
{
  std::ostringstream infoStream{};
  infoStream << "Task name: " << name.toStdString() << std::endl
             << "Thread id: " << std::this_thread::get_id() << std::endl
             << "Complete!" << std::endl;
  std::this_thread::sleep_for(1000ms);
  return QString::fromStdString(infoStream.str());
}
