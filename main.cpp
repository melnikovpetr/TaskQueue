#include "TaskLauncher.h"

#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>

#include <QtCore/QEvent>

#include <iostream>
#include <sstream>

class TaskEvent : public QEvent
{
public:
  static int taskEndEventType()
  {
    static auto type = QEvent::registerEventType();
    return type;
  }

  static void taskEndEventFn(QObject* reseiver, TaskId taskId, TaskReturn&& taskReturn)
  {
    QCoreApplication::postEvent(
      reseiver, new TaskEvent{ taskEndEventType(), taskId, std::any_cast<QVariant>(taskReturn) });
  }

public:
  TaskEvent(int type, TaskId taskId, const QVariant& taskInfo)
    : QEvent(static_cast<QEvent::Type>(type))
    , _taskId{ _taskId }
    , _taskInfo{ taskInfo }
  {
  }

  TaskId taskId() const { return _taskId; }
  const QVariant& taskInfo() const { return _taskInfo; }

private:
  TaskId _taskId;
  QVariant _taskInfo;
};

int main(int argc, char* argv[])
{
  if (getenv("QTDIR"))
    QApplication::addLibraryPath(QString() + getenv("QTDIR") + "/plugins");

  QApplication a{ argc, argv };
  class ResultReceiver : public QObject
  {
  public:
    using QObject::QObject;
    bool event(QEvent* event) override
    {
      if (event->type() == TaskEvent::taskEndEventType())
        QMessageBox::information(nullptr, "Inform", static_cast<TaskEvent*>(event)->taskInfo().toString());
      return true;
    }
  } resultReceiver{};

  TaskLauncher launcher{ std::bind(
    TaskEvent::taskEndEventFn, &resultReceiver, std::placeholders::_1, std::placeholders::_2) };

  launcher.start();

  auto taskFn = [](const QString& name) -> QVariant
  {
    std::ostringstream infoStream{};
    infoStream << "Task name: " << name.toStdString() << std::endl
               << "Thread id: " << std::this_thread::get_id() << std::endl
               << "Complete!" << std::endl;
    return QString::fromStdString(infoStream.str());
  };

  for (auto& num : { 1, 2, 4, 5, 6, 7, 8, 9 })
    launcher.queueTask(taskFn, "Task " + QString::number(num));

  a.exec();
  return 0;
}