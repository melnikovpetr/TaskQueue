#include "TaskLauncher.h"

#include "TaskQueue.h"

#include <cassert>

TaskLauncher::TaskLauncher(const TaskEndEventFn& taskEndEventFn, ThreadCount threadCount)
  : _taskQueue{ std::make_unique<TaskQueue>() }
  , _taskThreads{ threadCount }
  , _taskEndEventFn{ taskEndEventFn }
{
}

TaskLauncher::~TaskLauncher()
{
  finish();
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
  for (auto& taskThread : _taskThreads)
    if (taskThread.joinable())
      queueTask(finishTaskId(), {});
  for (auto& taskThread : _taskThreads)
    taskThread.join();
}

void TaskLauncher::start()
{
  for (auto& taskThread : _taskThreads)
    if (!taskThread.joinable())
      taskThread.swap(std::thread{ [this]()
        {
          while (true)
          {
            auto& task = _taskQueue->pop();
            if (task.first == finishTaskId())
              break;
            task.second();
          }
        } });
}

ThreadCount TaskLauncher::threadCount() const
{
  return _taskThreads.size();
}

size_t TaskLauncher::taskCount() const
{
  return _taskQueue->size();
}

void TaskLauncher::queueTask(TaskId taskId, TaskFn&& taskFn)
{
  _taskQueue->push({ taskId, taskFn });
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
