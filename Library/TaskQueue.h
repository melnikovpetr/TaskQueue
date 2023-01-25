#ifndef TASK_QUEUE_H
#define TASK_QUEUE_H

#include "SpinMutex.h"

#include <condition_variable>
#include <deque>
#include <functional>

using TaskId = long long;
using TaskFn = std::function<void(void)>;
using TaskAwaiter = std::function<void(void)>;

struct Task
{
  TaskId taskId;
  TaskFn taskFn;
  TaskAwaiter taskAwaiter;
};

class TaskQueue
{
public:
  TaskQueue();
  Task pop();
  void push(Task&& task);
  bool isStarted() const noexcept;
  void stop() noexcept;
  void start() noexcept;
  void clear() noexcept;
  size_t size() const noexcept;

private:
  mutable SpinMutex _isBusy;
  std::deque<Task> _queue;
  std::condition_variable_any _taskCV;
  std::atomic<bool> _started;
};

#endif // TASK_QUEUE_H
