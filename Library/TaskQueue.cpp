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

bool TaskQueue::isStarted() const noexcept
{
  return _started.load(std::memory_order_relaxed);
}

void TaskQueue::stop() noexcept
{
  _started.store(false, std::memory_order_release);
}

void TaskQueue::start() noexcept
{
  _started.store(true, std::memory_order_seq_cst);
  _taskCV.notify_all();
}

void TaskQueue::clear() noexcept
{
  {
    std::unique_lock spinLock{ _isBusy };
    _queue.clear();
  }
}

size_t TaskQueue::size() const noexcept
{
  size_t result{};
  {
    std::unique_lock spinLock{ _isBusy };
    result = _queue.size();
  }
  return result;
}
