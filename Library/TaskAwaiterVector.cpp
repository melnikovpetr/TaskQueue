#include "TaskAwaiterVector.h"

#include <mutex>

TaskAwaiterVector::TaskAwaiterVector(size_t awaiterCount)
  : _isBusy{}
  , _vector{ awaiterCount }
{
}

void TaskAwaiterVector::set(size_t awaiterIndex, const TaskAwaiter& awaiter)
{
  TaskAwaiter awaiterCopy{ awaiter };
  {
    std::unique_lock spinLock{ _isBusy };
    _vector[awaiterIndex].swap(awaiterCopy);
  }
}

void TaskAwaiterVector::clear(size_t awaiterIndex) noexcept
{
  TaskAwaiter awaiter{};
  {
    std::unique_lock spinLock{ _isBusy };
    _vector[awaiterIndex].swap(awaiter);
  }
}

void TaskAwaiterVector::wait(size_t awaiterIndex) const
{
  TaskAwaiter awaiter{};
  {
    std::unique_lock spinLock{ _isBusy };
    if (_vector[awaiterIndex])
      awaiter = _vector[awaiterIndex];
  }
  if (awaiter)
    awaiter();
}

void TaskAwaiterVector::wait() const
{
  for (size_t awaiterIndex = 0; awaiterIndex < _vector.size(); ++awaiterIndex)
    wait(awaiterIndex);
}
