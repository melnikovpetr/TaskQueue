#ifndef TASK_QUEUE_H
#define TASK_QUEUE_H

#include <QtCore/QVariant>

#include <deque>
#include <functional>
#include <future>

typedef std::promise<QVariant>                   TaskPromise;
typedef std::function<QVariant(const QVariant&)> TaskFn;
typedef std::shared_future<QVariant>             TaskResult;

struct Task
{
	QVariant    id;
	TaskPromise promise;
	TaskResult  result;
	TaskFn      fn;
	QVariant    arg;
	bool        sendResult;
};

class TaskQueue
{
public:
	TaskQueue();
	Task pop();
	void push(Task&& task);
	void clear();
	size_t size() const;

private:
	mutable std::atomic_flag _isBusy;
	std::deque<Task>         _queue;
};

#endif // TASK_QUEUE_H