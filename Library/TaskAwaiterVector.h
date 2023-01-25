#ifndef TASK_AWAITER_VECTOR_H
#define TASK_AWAITER_VECTOR_H

#include "SpinMutex.h"

#include <functional>
#include <vector>

using TaskAwaiter = std::function<void(void)>;

class TaskAwaiterVector
{
public:
  TaskAwaiterVector(size_t awaiterCount);
  void set(size_t awaiterIndex, const TaskAwaiter& taskAwaiter);
  void clear(size_t awaiterIndex) noexcept;
  void wait(size_t awaiterIndex) const;
  void wait() const;

private:
  mutable SpinMutex _isBusy;
  std::vector<TaskAwaiter> _vector;
};

#endif // TASK_AWAITER_VECTOR_H
