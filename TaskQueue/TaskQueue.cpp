#include "TaskQueue.h"

#include <chrono>

TaskQueue::TaskQueue() : _isBusy{ATOMIC_FLAG_INIT}, _queue{} {}

Task TaskQueue::pop()
{
	Task task{};
	while (_isBusy.test_and_set(std::memory_order_acquire));
	task = std::move(_queue.back());
	_queue.pop_back();
	_isBusy.clear(std::memory_order_release);
	return task;
}

void TaskQueue::push(Task&& task)
{
	while (_isBusy.test_and_set(std::memory_order_acquire));
	_queue.push_back(std::move(task));
	_isBusy.clear(std::memory_order_release);
}

void TaskQueue::clear()
{
	while (_isBusy.test_and_set(std::memory_order_acquire));
	_queue.clear();
	_isBusy.clear(std::memory_order_release);
}

size_t TaskQueue::size() const
{
	size_t result{};
	while (_isBusy.test_and_set(std::memory_order_acquire));
	result = _queue.size();
	_isBusy.clear(std::memory_order_release);
	return result;
}
