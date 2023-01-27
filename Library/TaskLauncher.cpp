#include "TaskLauncher.h"

#include "TaskAwaiterVector.h"
#include "TaskQueue.h"

#include <cassert>

TaskLauncher::TaskLauncher(ThreadCount threadCount)
  : _taskQueue{ std::make_unique<TaskQueue>() }
  , _taskThreads{ threadCount }
  , _taskAwaiterVector{ std::make_unique<TaskAwaiterVector>(threadCount) }
  , _stopStartMutex{}
{

  for (size_t threadIndex = 0; threadIndex < _taskThreads.size(); ++threadIndex)
    if (!_taskThreads[threadIndex].joinable())
      std::thread{
        [this, threadIndex]()
        {
          while (true)
          {
            auto task{ _taskQueue->pop() };
            if (task.taskId == finishTaskId())
              break;
            _taskAwaiterVector->set(threadIndex, task.taskAwaiter);
            task.taskFn();
            _taskAwaiterVector->clear(threadIndex);
          }
        }
      }.swap(_taskThreads[threadIndex]);
}

TaskLauncher::~TaskLauncher()
{
  std::vector finishTasks(threadCount(), Task{ finishTaskId(), TaskFn{}, TaskAwaiter{} });
  {
    std::unique_lock stopStartLock{ _stopStartMutex };
    _taskQueue->clearAndPush(std::move(finishTasks));
    if (!_taskQueue->isStarted())
      _taskQueue->start();
    for (auto& taskThread : _taskThreads)
      if (taskThread.joinable())
        taskThread.join();
  }
}

void TaskLauncher::clear() noexcept
{
  _taskQueue->clear();
}

void TaskLauncher::stop() noexcept
{
  std::unique_lock stopStartLock{ _stopStartMutex }; // prevents stopping during launcher destruction
  _taskQueue->stop();
}

void TaskLauncher::stopAndWait(std::atomic<bool>* interruptFlag)
{
  {
    std::unique_lock stopStartLock{ _stopStartMutex };
    _taskQueue->stop();
    if (interruptFlag)
      *interruptFlag = true;
    _taskAwaiterVector->wait();
  }
}

void TaskLauncher::start()
{
  std::unique_lock stopStartLock{ _stopStartMutex };
  _taskQueue->start();
}

ThreadCount TaskLauncher::threadCount() const noexcept
{
  return _taskThreads.size();
}

size_t TaskLauncher::taskCount() const noexcept
{
  return _taskQueue->size();
}

void TaskLauncher::queueTask(TaskId taskId, TaskFn&& taskFn, TaskAwaiter&& taskAwaiter)
{
  _taskQueue->push({ taskId, taskFn, taskAwaiter });
}

TaskId TaskLauncher::finishTaskId() noexcept
{
  static auto taskId = generateTaskId();
  return taskId;
}

TaskId TaskLauncher::generateTaskId() noexcept
{
  return std::chrono::steady_clock::now().time_since_epoch().count();
}
