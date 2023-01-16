#ifndef TASK_LAUNCHER_H
#define TASK_LAUNCHER_H

#include "TaskQueueExport.h"

#include <functional>
#include <future>
#include <memory>
#include <unordered_map>

using ThreadCount = decltype(std::thread::hardware_concurrency());

class TaskQueue;

using TaskId = long long;

template<typename TResult>
using TaskResult = std::shared_future<TResult>;

template<typename TResult>
struct TaskHandle
{
  TaskId id;
  TaskResult<TResult> result;
};

using TaskFn = std::function<void(void)>;
using TaskWaiterFn = std::function<void(void)>;

template<typename TResult>
using TaskEndEventFn = std::function<void(TaskId, const TaskResult<TResult>&)>;

class TASKQUEUE_EXPORT TaskLauncher
{
public:
  TaskLauncher(ThreadCount threadCount = std::thread::hardware_concurrency());
  ~TaskLauncher();

  template<typename TFn, typename... TArgs, typename TResult = std::invoke_result_t<TFn, TaskId, TArgs...>>
  TaskHandle<TResult> queueTask(TFn&& fn, TArgs&&... args)
  {
    return queueTask(TaskEndEventFn<TResult>{}, std::forward<TFn>(fn), std::forward<TArgs>(args)...);
  }

  template<typename TFn, typename... TArgs, typename TResult = std::invoke_result_t<TFn, TaskId, TArgs...>>
  TaskHandle<TResult> queueTask(const TaskEndEventFn<TResult>& taskEndEventFn, TFn&& fn, TArgs&&... args)
  {
    auto taskId = generateTaskId();
    auto task = std::make_shared<std::packaged_task<TResult()>>(
      std::bind(std::forward<TFn>(fn), taskId, std::forward<TArgs>(args)...));
    TaskHandle<TResult> taskHandle{ taskId, task->get_future().share() };

    queueTask(
      taskHandle.id,
      [task, taskHandle, taskEndEventFn]() {
        task->operator()();
        if (taskEndEventFn)
          taskEndEventFn(taskHandle.id, taskHandle.result);
      },
      [result = taskHandle.result]() { result.wait(); });
    return taskHandle;
  }

  template<typename TFn, typename TResult = std::invoke_result_t<TFn, TaskId, size_t, size_t>>
  std::vector<TaskHandle<TResult>> queueBatch(
    size_t first, size_t last, size_t grain, TFn&& fn, const TaskEndEventFn<TResult>& taskEndEventFn = {})
  {
    std::vector<TaskHandle<TResult>> taskHandles{};
    auto count = last - first;
    if (count == 0)
      return taskHandles;
    if (grain >= count)
    {
      taskHandles.push_back(queueTask(std::forward<TFn>(fn), first, last));
    }
    else
    {
      if (grain == 0)
        grain = count / threadCount() + (count % threadCount() ? 1 : 0);
      taskHandles.reserve(count % grain ? (count / grain) + 1 : count / grain);
      for (auto from = first; from < last; from += grain)
      {
        auto to = std::min(last, from + grain);
        taskHandles.push_back(queueTask(taskEndEventFn, std::forward<TFn>(fn), from, to));
      }
    }
    return taskHandles;
  }

  void clear();
  void stop();
  void stopAndWait(std::atomic<bool>* interruptFlag = nullptr);
  void start();
  ThreadCount threadCount() const;
  size_t taskCount() const;

protected:
  static TaskId generateTaskId();
  static TaskId finishTaskId();

protected:
  void queueTask(TaskId taskId, TaskFn&& taskFn, TaskWaiterFn&& taskWaiterFn);

private:
  std::unique_ptr<TaskQueue> _taskQueue;
  std::vector<std::thread> _taskThreads;
  std::unordered_map<std::thread::id, TaskWaiterFn> _lastTaskWaiters;
};

#endif // TASK_LAUNCHER_H
