#include "TaskQueue.h"

SpinLock::SpinLock(std::atomic_flag& flag)
  : _flag{ std::addressof(flag) }
{
  while (_flag->test_and_set(std::memory_order_acquire))
    ;
}

SpinLock::~SpinLock()
{
  _flag->clear(std::memory_order_release);
}

TaskQueue::TaskQueue()
  : _isBusy{ ATOMIC_FLAG_INIT }
  , _queue{}
{
}

Task TaskQueue::pop()
{
  Task task{};
  if (std::unique_lock taskQueueLock{ _taskMutex })
  {
    _taskCV.wait(taskQueueLock, [this]() { return size(); });
    {
      SpinLock spinLock{ _isBusy };
      task = std::move(_queue.back());
      _queue.pop_back();
    }
  }
  return task;
}

void TaskQueue::push(Task&& task)
{
  {
    SpinLock spinLock{ _isBusy };
    _queue.push_back(std::move(task));
  }
  _taskCV.notify_one();
}

void TaskQueue::clear()
{
  if (std::unique_lock taskQueueLock{ _taskMutex })
  {
    SpinLock spinLock{ _isBusy };
    _queue.clear();
  }
}

size_t TaskQueue::size() const
{
  size_t result{};
  {
    SpinLock spinLock{ _isBusy };
    result = _queue.size();
  }
  return result;
}
