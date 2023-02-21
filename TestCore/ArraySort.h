#ifndef ARRAY_SORT_H
#define ARRAY_SORT_H

#include <TaskLauncher.h>

#include <unordered_map>
#include <vector>

using ArrayValue = int;
using Array = std::vector<ArrayValue>;
using ThreadId = std::thread::id;

using IndexRange = std::pair<size_t, size_t>;
using SortTaskProgress = double;
using SortTaskResult = TaskResult<std::string>;
using SortTaskInfo = std::pair<ThreadId, IndexRange>;
using SortTaskIndex = size_t;
using SortTaskStatus = std::pair<SortTaskIndex, bool>;

using SortStartEventFn = std::function<void(TaskId, ThreadId, size_t, size_t)>;
using SortProgressEventFn = std::function<void(TaskId, SortTaskProgress)>;
using SortEndEventFn = TaskEndEventFn<std::string>;

class ArraySort
{
public:
  static size_t availableSystemMemory();
  static size_t operCount(size_t arraySize) noexcept { return 2 * arraySize * std::log(arraySize); }
  static size_t minArraySize(ThreadCount threadCount) noexcept;
  // 80% of available memory
  static size_t maxArraySize() noexcept { return 0.8 * availableSystemMemory() / sizeof(ArrayValue); }
  static std::string threadIdToStr(ThreadId threadId);
  static size_t progressGrain(size_t operCount) noexcept { return operCount >= 20 ? operCount / 20 : 1; }

public:
  ArraySort(SortStartEventFn&& taskStartEventFn, SortProgressEventFn&& taskProgressEventFn, SortEndEventFn&& taskEndEventFn);
  ~ArraySort() { _taskLauncher.stopAndWait(&_interruptFlag); }

  auto threadCount() const noexcept { return _taskLauncher.threadCount(); }
  auto minArraySize() const noexcept { return minArraySize(threadCount()); }
  auto arraySize() const noexcept { return _array.size(); }
  void sort();
  void generateArray(size_t arraySize);
  void interrupt();

  auto saveTask(TaskId taskId) { return _taskIndexes.insert({ taskId, { _taskIndexes.size(), false } }).first->second.first; }
  auto clearTasks() noexcept { _taskIndexes.clear(); }
  SortTaskIndex finishTask(TaskId taskId);
  auto taskCount() const noexcept { return _taskIndexes.size(); }
  auto isTaskSaved(TaskId taskId) const { return _taskIndexes.find(taskId) != _taskIndexes.cend(); }
  auto taskIndex(TaskId taskId) const { return _taskIndexes.at(taskId).first; }
  auto isTaskFinished(TaskId taskId) const { return _taskIndexes.at(taskId).second; }
  bool areAllTasksFinished() const;

private:
  TaskLauncher _taskLauncher;
  Array _array;
  std::atomic<bool> _interruptFlag;
  std::unordered_map<TaskId, std::pair<size_t, bool>> _taskIndexes;
  SortStartEventFn _taskStartEventFn;
  SortProgressEventFn _taskProgressEventFn;
  SortEndEventFn _taskEndEventFn;
};

#endif // ARRAY_SORT_H
