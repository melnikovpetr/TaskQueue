#ifndef THREAD_SAFE_QUEUE_H
#define THREAD_SAFE_QUEUE_H

#include "SpinMutex.h"

#include <deque>
#include <mutex>

template <typename T>
class ThreadSafeQueue
{
public:
  ThreadSafeQueue() = default;
  bool tryPop(T& value);
  void push(T&& value);
  void clear() noexcept;
  size_t size() const noexcept;

  ThreadSafeQueue(const ThreadSafeQueue&) = delete;
  ThreadSafeQueue(ThreadSafeQueue&&) = delete;
  ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;
  ThreadSafeQueue& operator=(ThreadSafeQueue&&) = delete;

private:
  mutable SpinMutex _isBusy;
  std::deque<T> _queue;
};

template <typename T>
inline bool ThreadSafeQueue<T>::tryPop(T& value)
{
  std::unique_lock spinLock{ _isBusy };
  if (_queue.size())
  {
    value = std::move(_queue.front());
    _queue.pop_front();
    return true;
  }
  return false;
}

template <typename T>
inline void ThreadSafeQueue<T>::push(T&& task)
{
  std::unique_lock spinLock{ _isBusy };
  _queue.push_back(std::move(task));
}

template <typename T>
inline void ThreadSafeQueue<T>::clear() noexcept
{
  std::unique_lock spinLock{ _isBusy };
  _queue.clear();
}

template <typename T>
inline size_t ThreadSafeQueue<T>::size() const noexcept
{
  std::unique_lock spinLock{ _isBusy };
  return _queue.size();
}

#endif // THREAD_SAFE_QUEUE_H
