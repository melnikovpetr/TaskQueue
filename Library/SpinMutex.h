#ifndef SPIN_MUTEX_H
#define SPIN_MUTEX_H

#include "TaskQueueExport.h"

#include <atomic>

class TASKQUEUE_EXPORT SpinMutex
{
public:
  SpinMutex() noexcept = default;
  void lock() noexcept;
  void unlock() noexcept;
  bool try_lock() noexcept;

private:
  std::atomic<bool> _spin{ false };
};

#endif // SPIN_MUTEX_H
