#ifndef TASK_LAUNCHER_H
#define TASK_LAUNCHER_H

#include <QtCore/QVariant> 
#include <QtCore/QEvent> 

#include <memory>
#include <future>

typedef std::function<QVariant(const QVariant&)> TaskFn;
typedef std::shared_future<QVariant>             TaskResult;

struct TaskHandle
{
	QVariant   id;
	TaskResult result;
};

class TaskEvent : public QEvent
{
public:
	static const int taskEventType;

public:
	TaskEvent(const TaskHandle& taskHandle);
	const TaskHandle& taskHandle() const { return _taskHandle; }

private:
	TaskHandle _taskHandle;
};

class TaskLauncher
{
public:
	TaskLauncher(QObject* resultReceiver = nullptr);
	~TaskLauncher();

	TaskHandle queueTask(const TaskFn& taskFn, const QVariant& taskArg = {}, bool sendResult = true);
	void clear();
	void stop();
	void finish();
	void start();
	void wait();

protected:
	static QVariant generateTaskId();
	static QVariant finishTaskId();

protected:
	TaskHandle queueTask(class Task&& task);

private:
	std::unique_ptr<class TaskQueue> _taskQueue;
	std::thread                      _taskThread;
	std::mutex						 _taskMutex;
	std::unique_lock<std::mutex>     _taskLock;
	std::condition_variable	         _taskCV;
	QObject*                         _resultReceiver;
};

#endif // TASK_LAUNCHER_H