#ifndef TASK_QUEUE_H
#define TASK_QUEUE_H

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>

using TaskId = long long;
using TaskFn = std::function<void(void)>;
using TaskWaiterFn = std::function<void(void)>;

struct Task
{
  TaskId taskId;
  TaskFn taskFn;
  TaskWaiterFn taskWaiterFn;
};

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
  Task pop(TaskWaiterFn& taskWaiterFn);
  auto lockPopping() { return std::unique_lock{ _taskMutex }; }
  void push(Task&& task);
  bool isStarted() const;
  void stop();
  void start();
  void clear();
  size_t size() const;

private:
  mutable std::atomic_flag _isBusy;
  std::deque<Task> _queue;
  std::mutex _taskMutex;
  std::condition_variable _taskCV;
  std::atomic<bool> _started;
};

#endif // TASK_QUEUE_H
