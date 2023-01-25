#include "SpinMutex.h"

#include <immintrin.h>

void SpinMutex::lock() noexcept
{
  while (_spin.exchange(true, std::memory_order_acquire))
    while (_spin.load(std::memory_order_relaxed))
      _mm_pause();
}

void SpinMutex::unlock() noexcept
{
  _spin.store(false, std::memory_order_release);
}

bool SpinMutex::try_lock() noexcept
{
  return !_spin.load(std::memory_order_relaxed) && !_spin.exchange(true, std::memory_order_acquire);
}
