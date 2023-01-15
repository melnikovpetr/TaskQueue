#ifndef TASK_LAUNCHER_H
#define TASK_LAUNCHER_H

#include "TaskQueueExport.h"

#include <any>
#include <functional>
#include <future>
#include <memory>

using ThreadCount = decltype(std::thread::hardware_concurrency());

class TaskQueue;

using TaskId = long long;
using TaskFn = std::function<void()>;
using TaskReturn = std::any;
using TaskEndEventFn = std::function<void(TaskId, TaskReturn&&)>;

template<typename TReturn>
using TaskResult = std::shared_future<TReturn>;

template<typename TReturn>
struct TaskHandle
{
  TaskId id;
  TaskResult<TReturn> result;

  std::any resultValue() const
  {
    if constexpr (std::is_same_v<TReturn, void>)
      return result.valid() ? (result.get(), std::any{}) : std::any{};
    else
      return result.valid() ? result.get() : std::any{};
  };
};

class TASKQUEUE_EXPORT TaskLauncher
{
public:
  TaskLauncher(
    const TaskEndEventFn& taskEndEventFn = {}, ThreadCount threadCount = std::thread::hardware_concurrency());
  ~TaskLauncher();

  template<typename TFn, typename... TArgs>
  TaskHandle<std::result_of_t<TFn(TaskId id, TArgs...)>> queueTask(TFn&& fn, TArgs&&... args)
  {
    using TReturn = std::result_of_t<TFn(TaskId id, TArgs...)>;
    using THandle = TaskHandle<TReturn>;
    auto taskId = generateTaskId();
    auto task = std::make_shared<std::packaged_task<TReturn()>>(
      std::bind(std::forward<TFn>(fn), taskId, std::forward<TArgs>(args)...));
    THandle taskHandle{ taskId, task->get_future().share() };

    queueTask(taskHandle.id, [task, taskHandle, taskEndEventFn = _taskEndEventFn]() {
      task->operator()();
      if (taskEndEventFn)
        taskEndEventFn(taskHandle.id, taskHandle.resultValue());
    });
    return taskHandle;
  }

  template<typename TFn>
  std::vector<TaskHandle<std::result_of_t<TFn(TaskId id, size_t, size_t)>>> queueBatch(
    size_t first, size_t last, size_t grain, TFn&& fn)
  {
    decltype(queueBatch(first, last, grain, std::forward<TFn>(fn))) result{};
    auto count = last - first;
    if (count == 0)
      return result;
    if (grain >= count)
    {
      result.push_back(queueTask(std::forward<TFn>(fn), first, last));
    }
    else
    {
      if (grain == 0)
        grain = count / threadCount() + (count % threadCount() ? 1 : 0);
      result.reserve(count % grain ? (count / grain) + 1 : count / grain);
      for (auto from = first; from < last; from += grain)
      {
        auto to = std::min(last, from + grain);
        result.push_back(queueTask(std::forward<TFn>(fn), from, to));
      }
    }
    return result;
  }

  void clear();
  void stop();
  void finish();
  void start();
  ThreadCount threadCount() const;
  size_t taskCount() const;

protected:
  static TaskId generateTaskId();
  static TaskId finishTaskId();

protected:
  void queueTask(TaskId taskId, TaskFn&& taskFn);

private:
  std::unique_ptr<TaskQueue> _taskQueue;
  std::vector<std::thread> _taskThreads;
  TaskEndEventFn _taskEndEventFn;
};

#endif // TASK_LAUNCHER_H
