#ifndef TASK_QUEUE_H
#define TASK_QUEUE_H

#include <atomic>
#include <deque>
#include <functional>
#include <mutex>

using TaskId = long long;
using TaskFn = std::function<void()>;
using Task = std::pair<TaskId, TaskFn>;

class SpinLock
{
public:
  SpinLock(std::atomic_flag& flag);
  ~SpinLock();

private:
  std::atomic_flag* _flag;
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
  std::deque<Task> _queue;
  std::mutex _taskMutex;
  std::condition_variable _taskCV;
};

#endif // TASK_QUEUE_H