#include "TaskLauncher.h"

#include "TaskQueue.h"

#include <cassert>

TaskLauncher::TaskLauncher(ThreadCount threadCount)
  : _taskQueue{ std::make_unique<TaskQueue>() }
  , _taskThreads{ threadCount }
  , _lastTaskWaiters{ threadCount }
{
  for (auto& taskThread : _taskThreads)
    if (!taskThread.joinable())
      std::thread{
        [this]() {
          if (auto taskQueueLock = _taskQueue->lockPopping())
            _lastTaskWaiters.insert({ std::this_thread::get_id(), TaskWaiterFn{} });
          while (true)
          {
            auto task{ _taskQueue->pop(_lastTaskWaiters.at(std::this_thread::get_id())) };
            if (task.taskId == finishTaskId())
              break;
            task.taskFn();
          }
        }
      }.swap(taskThread);
}

TaskLauncher::~TaskLauncher()
{
  clear();
  for (auto& taskThread : _taskThreads)
    if (taskThread.joinable())
      queueTask(finishTaskId(), TaskFn{}, TaskWaiterFn{});
  if (!_taskQueue->isStarted())
    start();
  for (auto& taskThread : _taskThreads)
    taskThread.join();
}

void TaskLauncher::clear()
{
  _taskQueue->clear();
}

void TaskLauncher::stop()
{
  _taskQueue->stop();
}

void TaskLauncher::stopAndWait(std::atomic<bool>* interruptFlag)
{
  if (auto taskQueueLock = _taskQueue->lockPopping())
  {
    _taskQueue->stop();
    if (interruptFlag)
      *interruptFlag = true;
    for (auto& taskWaiter : _lastTaskWaiters)
      if (taskWaiter.second)
        taskWaiter.second();
  }
}

void TaskLauncher::start()
{
  _taskQueue->start();
}

ThreadCount TaskLauncher::threadCount() const
{
  return _taskThreads.size();
}

size_t TaskLauncher::taskCount() const
{
  return _taskQueue->size();
}

void TaskLauncher::queueTask(TaskId taskId, TaskFn&& taskFn, TaskWaiterFn&& taskWaiterFn)
{
  _taskQueue->push({ taskId, taskFn, taskWaiterFn });
}

TaskId TaskLauncher::finishTaskId()
{
  static auto taskId = generateTaskId();
  return taskId;
}

TaskId TaskLauncher::generateTaskId()
{
  return std::chrono::steady_clock::now().time_since_epoch().count();
}
