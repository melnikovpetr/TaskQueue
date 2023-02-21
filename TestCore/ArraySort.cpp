#include "ArraySort.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <random>
#include <sstream>

#ifdef WIN32
#include <windows.h>
size_t ArraySort::availableSystemMemory()
{
  MEMORYSTATUSEX statex;
  statex.dwLength = sizeof(statex);
  ::GlobalMemoryStatusEx(&statex);
  return statex.ullAvailPhys;
}
#else
//#include <unistd.h>
// static size_t availableSystemMemory()
//{
//  return ::sysconf(_SC_PAGE_SIZE) * ::sysconf(_SC_AVPHYS_PAGES);
//}
#include <fstream>
size_t ArraySort::availableSystemMemory()
{
  std::string title{};
  size_t value{ 0 };
  std::string units{};
  std::ifstream file{ "/proc/meminfo" };
  while (file >> title)
  {
    if (title == "MemAvailable:")
      return (file >> value) ? ((file >> units) ? (units == "kB" ? value * 1024 : 0) : 0) : 0;
    // Ignore the rest of the line
    file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }
  return 0; // Nothing found
}
#endif

size_t ArraySort::minArraySize(ThreadCount threadCount) noexcept
{
  assert(threadCount > 0);
  return static_cast<size_t>(threadCount) * 2;
}

std::string ArraySort::threadIdToStr(ThreadId threadId)
{
  std::ostringstream ss{};
  ss << threadId;
  return ss.str();
}

ArraySort::ArraySort(SortStartEventFn&& taskStartEventFn, SortProgressEventFn&& taskProgressEventFn, SortEndEventFn&& taskEndEventFn)
  : _taskLauncher{}
  , _array(minArraySize(_taskLauncher.threadCount()))
  , _interruptFlag{ false }
  , _taskStartEventFn{ std::move(taskStartEventFn) }
  , _taskProgressEventFn{ std::move(taskProgressEventFn) }
  , _taskEndEventFn{ std::move(taskEndEventFn) }
{
  assert(_taskStartEventFn && _taskProgressEventFn && _taskEndEventFn);
  generateArray(_array.size());
}

SortTaskIndex ArraySort::finishTask(TaskId taskId)
{
  auto& taskIndex = _taskIndexes.at(taskId);
  taskIndex.second = true;
  return taskIndex.first;
}

bool ArraySort::areAllTasksFinished() const
{
  auto result = !taskCount() || (taskCount() == threadCount());
  for (const auto& taskIndex : _taskIndexes)
    if (!(result = result && taskIndex.second.second))
      break;
  return result;
}

void ArraySort::sort()
{
  clearTasks();
  _taskLauncher.queueBatch(
    0, _array.size(), 0,
    [this](TaskId taskId, size_t from, size_t to) -> std::string
    {
      auto threadOperIndex = size_t(0);
      auto threadOperCount = operCount(to - from);
      auto progressGrain = ArraySort::progressGrain(threadOperCount);
      auto threadCmpPred = [this, taskId, &threadOperIndex, threadOperCount, progressGrain](const ArrayValue& l, const ArrayValue& r) mutable -> bool
      {
        if (threadOperIndex && !(threadOperIndex % progressGrain))
        {
          auto progress = static_cast<SortTaskProgress>(threadOperIndex) / threadOperCount;
          _taskProgressEventFn(taskId, progress >= 0.99 ? 0.99 : progress);
          if (_interruptFlag)
            throw std::runtime_error("Task with id = " + std::to_string(taskId) + " was interrupted!");
        }
        threadOperIndex++;
        return l < r;
      };
      _taskStartEventFn(taskId, std::this_thread::get_id(), from, to);
      std::sort(_array.begin() + from, _array.begin() + to, threadCmpPred);
      return "min = " + std::to_string(_array[from]) + ", max = " + std::to_string(_array[to - 1]);
    },
    _taskEndEventFn);
}

void ArraySort::interrupt()
{
  _taskLauncher.stopAndWait(&_interruptFlag);
  _taskLauncher.clear();
  _interruptFlag = false;
  _taskLauncher.start();
}

void ArraySort::generateArray(size_t arraySize)
{
  interrupt();
  decltype(_array){}.swap(_array);
  _array.resize(arraySize);
  auto taskHandles = _taskLauncher.queueBatch(0, _array.size(), 0,
    [this](TaskId taskId, size_t from, size_t to)
    {
      std::uniform_int_distribution<ArrayValue> distribution(0, (std::numeric_limits<ArrayValue>::max)());
      std::mt19937 randomEngine{ std::random_device{}() };
      auto random = std::bind(distribution, randomEngine);
      for (auto index = from; index < to; ++index)
        _array[index] = random();
    });
  for (auto& taskHandle : taskHandles)
    taskHandle.result.wait();
}
