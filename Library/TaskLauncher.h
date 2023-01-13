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
};

class TASKQUEUE_EXPORT TaskLauncher
{
public:
  TaskLauncher(
    const TaskEndEventFn& taskEndEventFn = {}, ThreadCount threadCount = std::thread::hardware_concurrency());
  ~TaskLauncher();

  template<typename TFn, typename... TArgs>
  TaskHandle<std::result_of_t<TFn(TArgs...)>> queueTask(TFn&& fn, TArgs&&... args)
  {
    using TReturn = std::result_of_t<TFn(TArgs...)>;
    using THandle = TaskHandle<TReturn>;
    auto task =
      std::make_shared<std::packaged_task<TReturn()>>(std::bind(std::forward<TFn>(fn), std::forward<TArgs>(args)...));
    THandle taskHandle{ generateTaskId(), task->get_future().share() };

    queueTask(taskHandle.id, [task, taskHandle, taskEndEventFn = _taskEndEventFn]() {
      task->operator()();
      if (taskEndEventFn && taskHandle.result.valid())
        taskEndEventFn(taskHandle.id, taskHandle.result.get());
    });
    return taskHandle;
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
