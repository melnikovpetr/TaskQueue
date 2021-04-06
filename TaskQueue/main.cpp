#include "TaskLauncher.h"

//#include <QtCore/QCoreApplication>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>

#include <iostream>

int main(int argc, char* argv[])
{
	if (getenv("QTDIR")) QApplication::addLibraryPath(QString() + getenv("QTDIR") + "/plugins");

	QApplication a{argc, argv};
	class ResultReceiver : public QObject
	{
	public:
		using QObject::QObject;
		bool event(QEvent* event) override
		{
			if (event->type() == TaskEvent::taskEventType)
			{
				QMessageBox::information(nullptr, "Inform", static_cast<TaskEvent*>(event)->taskHandle().result.get().toString());
			}
			return true;
		}
	} resultReceiver{};
	TaskLauncher launcher{&resultReceiver};

	launcher.start();

	launcher.queueTask([](const QVariant& name) {
		std::cout << "Task named " << name.toString().toStdString() << " is performed!" << std::endl;
		return name;
	}, "Task 1");

	a.exec();
	return 0;
}