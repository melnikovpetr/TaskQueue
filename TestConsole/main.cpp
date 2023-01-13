#include <chrono>
#include <iostream>
#include <sstream>

#include "TaskLauncher.h"

namespace bind_ph = std::placeholders;

using namespace std::chrono_literals;

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

  TaskLauncher launcher{ std::bind(TaskEvent::taskEndEventFn, &resultReceiver, bind_ph::_1, bind_ph::_2) };
  launcher.start();

  auto taskFn = [](const QString& name) -> QVariant {
    std::ostringstream infoStream{};
    infoStream << "Task name: " << name.toStdString() << std::endl
               << "Thread id: " << std::this_thread::get_id() << std::endl
               << "Complete!" << std::endl;
    std::this_thread::sleep_for(1000ms);
    return QString::fromStdString(infoStream.str());
  };

  for (auto& num : { 1, 2, 4, 5, 6, 7, 8, 9 })
    launcher.queueTask(taskFn, "Task " + QString::number(num));

  a.exec();
  return 0;
}
