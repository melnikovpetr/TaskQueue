#include "TestWidget.h"

#include "Keyboard.h"

#include <ArraySort.h>
#include <TaskLauncher.h>
#include <ThreadSafeQueue.h>

#include <iostream>
#include <sstream>
#include <variant>

#define WN_MAIN "Main"
#define WN_CMD "Cmd"
#define WN_TITLE "Title"
#define WN_TASK_INFO "TaskInfo"
#define WN_ARR_SZ "ArraySize"
#define WN_ARR_SZ_ERR "ArraySizeError"
#define WN_SZ_INFO "SizesInfo"

using namespace std::placeholders;

struct Event;

using EventInfo = std::variant<SortTaskProgress, SortTaskInfo, SortTaskResult>;
using EventQueue = ThreadSafeQueue<Event>;

struct Event
{
  TaskId taskId;
  EventInfo info;

  static EventQueue& queue()
  {
    static EventQueue _eventQueue{};
    return _eventQueue;
  }
  static bool get(Event& event) { return queue().tryPop(event); }
  static void post(Event&& event)
  {
    queue().push(std::move(event));
    if (!Widget::hasInput())
      Widget::ungetInput(WidgetInput::REFRESH);
  }

  static void postEnd(TaskId taskId, const SortTaskResult& result) { post({ taskId, { result } }); }
  static void postStart(TaskId taskId, ThreadId threadId, size_t begin, size_t end) { post({ taskId, SortTaskInfo{ threadId, { begin, end } } }); }
  static void postProgress(TaskId taskId, double progress) { post({ taskId, { progress } }); }
};

MainWidget::MainWidget()
  : ComposedWidget(WN_MAIN)
  , _taskLauncherInfo{ { "Id", "Thread Id", "Array Index Range", "Progress", "Result" } }
  , _sizesInfo{ { "Thread Pool Size", "Array Size" } }
  , _arraySort{ Event::postStart, Event::postProgress, Event::postEnd }
{
  _sizesInfo.appendRow();

  pushChild(createWidget(WN_TITLE));
  pushChild(createWidget(WN_SZ_INFO));
  pushChild(createWidget(WN_TASK_INFO));
  pushChild(createWidget(WN_CMD));
}

void MainWidget::draw()
{
  Event event{};

  _sizesInfo.setData(0, SI_THREAD_POOL, std::to_string(_arraySort.threadCount()));
  _sizesInfo.setData(0, SI_ARRAY, std::to_string(_arraySort.arraySize()));
  childWidget<TableWidget>(WN_SZ_INFO)->adjustWidth();

  while (Event::get(event))
  {
    if (std::holds_alternative<SortTaskInfo>(event.info))
    {
      auto& taskInfo = std::get<SortTaskInfo>(event.info);
      auto& threadId = taskInfo.first;
      auto& taskRange = taskInfo.second;

      auto row = _arraySort.saveTask(event.taskId);
      while (_taskLauncherInfo.rowCount() <= row)
        _taskLauncherInfo.appendRow();

      _taskLauncherInfo.setData(row, TLI_ID, std::to_string(event.taskId));
      _taskLauncherInfo.setData(row, TLI_THREAD_ID, ArraySort::threadIdToStr(threadId));
      _taskLauncherInfo.setData(row, TLI_INFO, "[" + std::to_string(taskRange.first) + ", " + std::to_string(taskRange.second) + ")");
      _taskLauncherInfo.setData(row, TLI_PROGRESS, std::to_string(0.0));
      _taskLauncherInfo.setData(row, TLI_RESULT, "");

      childWidget<TableWidget>(WN_TASK_INFO)->adjustWidth();
    }
    else if (std::holds_alternative<SortTaskProgress>(event.info) && _arraySort.isTaskSaved(event.taskId))
    {
      auto& taskInfo = std::get<SortTaskProgress>(event.info);
      auto row = _arraySort.taskIndex(event.taskId);
      if (row < _taskLauncherInfo.rowCount())
      {
        _taskLauncherInfo.setData(row, TLI_PROGRESS, std::to_string(taskInfo));
        childWidget<TableWidget>(WN_TASK_INFO)->adjustWidth();
      }
    }
    else if (std::holds_alternative<SortTaskResult>(event.info) && _arraySort.isTaskSaved(event.taskId))
    {
      auto& taskInfo = std::get<SortTaskResult>(event.info);
      auto row = _arraySort.finishTask(event.taskId);
      if (row < _taskLauncherInfo.rowCount())
      {
        try
        {
          auto& taskResult = taskInfo.get();
          _taskLauncherInfo.setData(row, TLI_PROGRESS, std::to_string(1.0));
          _taskLauncherInfo.setData(row, TLI_RESULT, taskResult);
        }
        catch (const std::runtime_error& e)
        {
          _taskLauncherInfo.setData(row, TLI_RESULT, e.what());
        }
        childWidget<TableWidget>(WN_TASK_INFO)->adjustWidth();
      }
    }
  }

  Keyboard::clearConsole();
  ComposedWidget::draw();
}

std::unique_ptr<Widget> MainWidget::createWidget(const std::string& name)
{
  if (name == WN_TITLE)
    return std::make_unique<InfoWidget>(name, "Test console\n\n");
  else if (name == WN_SZ_INFO)
    return std::make_unique<TableWidget>(name, "", &_sizesInfo);
  else if (name == WN_TASK_INFO)
    return std::make_unique<TableWidget>(name, "Running Tasks", &_taskLauncherInfo);
  else if (name == WN_CMD)
    return std::make_unique<ActionWidget>(name, "Type:",
      ActionWidget::Actions{ { '1', { "To resize and generate input array", std::bind(&MainWidget::readArraySize, this) } },
        { '2', { "To start/stop sorting", std::bind(&MainWidget::startStop, this) } }, { WidgetInput::ESC, { "To exit", {} } } });
  else if (name == WN_ARR_SZ)
    return std::make_unique<InputWidget>(name,
      "Enter the size of the array [" + std::to_string(ArraySort::minArraySize(_arraySort.threadCount())) + ", " +
        std::string(std::to_string(ArraySort::maxArraySize())) + "]: ",
      "[\\d]+", std::bind(&MainWidget::setArraySize, this, _1));
  else if (name == WN_ARR_SZ_ERR)
    return std::make_unique<ActionWidget>(
      name, "Incorrect array size!", ActionWidget::Actions{ { WidgetInput::NO_INPUT, { "", [this]() { removeChild(WN_ARR_SZ_ERR); } } } });
  else
    return std::unique_ptr<Widget>{};
}

void MainWidget::readArraySize()
{
  pushChild(createWidget(WN_ARR_SZ));
}

void MainWidget::startStop()
{
  if (_arraySort.areAllTasksFinished())
    _arraySort.sort();
  else
    _arraySort.interrupt();
}

void MainWidget::setArraySize(const std::string& value)
{
  size_t size;
  std::stringstream{ value } >> size;
  removeChild(child(WN_ARR_SZ));
  if ((_arraySort.minArraySize() <= size) && (size <= ArraySort::maxArraySize()))
  {
    std::cout << "Array generation! Please wait!\n";
    _arraySort.generateArray(size);
  }
  else
    pushChild(createWidget(WN_ARR_SZ_ERR));
}
