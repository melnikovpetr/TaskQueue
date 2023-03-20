#include "TaskQueue.h"

TaskQueue::TaskQueue()
  : _isBusy{}
  , _queue{}
  , _taskCV{}
  , _started{ true }
{
}

Task TaskQueue::pop()
{
  Task task{};
  {
    std::unique_lock spinLock{ _isBusy };
    _taskCV.wait(spinLock, [this]() noexcept { return isStarted() && _queue.size(); });
    task = std::move(_queue.back());
    _queue.pop_back();
  }
  return task;
}

void TaskQueue::push(Task&& task)
{
  {
    std::unique_lock spinLock{ _isBusy };
    _queue.push_back(std::move(task));
  }
  _taskCV.notify_one();
}

void TaskQueue::clearAndPush(std::vector<Task>&& tasks)
{
  {
    std::unique_lock spinLock{ _isBusy };
    _queue.clear();
    for (auto& task : tasks)
      _queue.push_back(std::move(task));
  }
  _taskCV.notify_all();
}

bool TaskQueue::isStarted() const noexcept
{
  return _started.load(std::memory_order_acquire);
}

void TaskQueue::stop() noexcept
{
  _started.store(false, std::memory_order_release);
}

void TaskQueue::start() noexcept
{
  _started.store(true, std::memory_order_release);
  _taskCV.notify_all();
}

void TaskQueue::clear() noexcept
{
  std::unique_lock spinLock{ _isBusy };
  _queue.clear();
}

size_t TaskQueue::size() const noexcept
{
  std::unique_lock spinLock{ _isBusy };
  return _queue.size();
}
