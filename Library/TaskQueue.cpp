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
  : _isBusy{}
  , _queue{}
  , _started{ true }
{
}

Task TaskQueue::pop(TaskWaiterFn& taskWaiterFn)
{
  Task task{};
  if (auto taskQueueLock = lockPopping())
  {
    _taskCV.wait(taskQueueLock, [this]() { return isStarted() && size(); });
    {
      SpinLock spinLock{ _isBusy };
      task = std::move(_queue.back());
      _queue.pop_back();
    }
    taskWaiterFn = task.taskWaiterFn;
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

bool TaskQueue::isStarted() const
{
  return _started.load(std::memory_order_relaxed);
}

void TaskQueue::stop()
{
  _started.store(false, std::memory_order_relaxed);
}

void TaskQueue::start()
{
  _started.store(true, std::memory_order_seq_cst);
  _taskCV.notify_all();
}

void TaskQueue::clear()
{
  if (auto taskQueueLock = lockPopping())
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
