#include "TaskLauncher.h"

#include "TaskQueue.h"

#include <QtCore/QVariant>
#include <QtCore/QCoreApplication>

#include <cassert>

const int TaskEvent::taskEventType = QEvent::registerEventType();

TaskEvent::TaskEvent(const TaskHandle& taskHandle)
	: QEvent(static_cast<QEvent::Type>(taskEventType)), _taskHandle{taskHandle.id, taskHandle.result}
{}

TaskLauncher::TaskLauncher(QObject* resultReceiver)
	: _taskQueue{std::make_unique<TaskQueue>()}
	, _taskThread{}
	, _taskMutex{}
	, _taskLock{_taskMutex, std::defer_lock}
	, _taskCV{}
	, _resultReceiver{resultReceiver}
{}

TaskLauncher::~TaskLauncher()
{
	finish();
}

TaskHandle TaskLauncher::queueTask(const TaskFn& taskFn, const QVariant& taskArg, bool sendResult)
{
	return queueTask({generateTaskId(), {}, {}, taskFn, taskArg, sendResult});
}

void TaskLauncher::clear()
{
	_taskQueue->clear();
}

void TaskLauncher::stop()
{
	clear();
	finish();
}

void TaskLauncher::finish()
{
	if (_taskThread.joinable())
	{
		queueTask({finishTaskId(), {}, {}, {}, {}, false});
		_taskThread.join();
	}
}

void TaskLauncher::start()
{
	if (_taskThread.joinable()) return;

	_taskThread.swap(std::thread{ [this]() {
		while (true)
		{
			_taskCV.wait(
				_taskLock,
				[this]() { 
					if (_taskQueue->size()) { if (_taskLock) _taskLock.unlock(); }
					else { if (!_taskLock) { _taskLock.lock(); } if (_taskQueue->size()) { _taskLock.unlock(); } }
					return !_taskLock;
				}
			);
			
			{
				Task& task = _taskQueue->pop();
				
				try
				{
					if (task.fn) task.promise.set_value(task.fn(task.arg));
					else task.promise.set_value({});
				}
				catch (...)
				{
					try { task.promise.set_exception(std::current_exception()); } catch (...) {}
				}

				if (task.sendResult && _resultReceiver)
					QCoreApplication::postEvent(_resultReceiver, new TaskEvent({task.id, task.result}));

				if (task.id == finishTaskId()) break;
			}
		}
	}});
}

void TaskLauncher::wait()
{
	queueTask({}, {}, false).result.wait();
}

QVariant TaskLauncher::finishTaskId()
{
	static QVariant taskId = generateTaskId();
	return taskId;
}

TaskHandle TaskLauncher::queueTask(Task&& task)
{
	TaskHandle taskHandle{task.id,  task.result = task.promise.get_future().share()};
	_taskQueue->push(std::move(task));
	if (_taskLock) std::lock_guard<std::mutex>{_taskMutex};
	_taskCV.notify_all();
	return taskHandle;
}

QVariant TaskLauncher::generateTaskId()
{
	return std::chrono::steady_clock::now().time_since_epoch().count();
}
