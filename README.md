# TaskQueue
TaskQueue is a library that implements an experimental thread pool with a shared task queue.
## API Reference
The library API is enclosed in the TaskLauncher class.
```
TaskLauncher
{
  // Creates a pool with threadCount threads.
  TaskLauncher(threadCount = coreCount);
  // Enqueues the task for execution, returns a descriptor (an object with the task id and the future result (std::shared_future)).
  TaskHandle queueTask(taskFn, taskFnArgs…);
  // Enqueues the task for execution, returns a descriptor. Additionally, it allows you to set a function that notifies about the completion of the task (callback).
  TaskHandle queueTask(notifyTaskEndFn,  taskFn, taskFnArgs…);
  // Enqueues the task batch for execution. The packet is formed by dividing the given interval [first, last) into segments with size of grain. Additionally, it allows you to set a function that notifies about the completion of each task in the batch.
  TaskHandles queueBatch(first, last, grain, taskFn, notifyTaskEndFn = {});
  // Clears the task queue.
  clear();
  // Stops popping tasks from the queue.
  stop();
  // Stops popping tasks from the queue, sets the task interruption flag if necessary, and waits for already running tasks to complete.
  stopAndWait(interruptFlag = {});
  // Starts popping tasks from the queue.
  start();
  // Number of threads in the pool.
  Count threadCount();
  // Number of tasks in the queue.
  Count taskCount();
}
```
## Code Example
```cpp
TaskLauncher launcher{};
launcher.stop();

auto taskFn = [](TaskId taskId, const std::string& name) -> std::string {
  std::ostringstream infoStream{};
  infoStream << "Task name: " << name << std::endl
             << "Task id: " << taskId << std::endl
             << "Thread id: " << std::this_thread::get_id() << std::endl
             << "Complete!" << std::endl;
  std::this_thread::sleep_for(1000ms);
  return infoStream.str();
};

auto nums = { 1, 2, 4, 5, 6, 7, 8, 9 };
std::vector<TaskHandle<decltype(taskFn({}, {}))>> taskHandles(nums.size());

for (decltype(nums.size()) taskIndex = 0; taskIndex < nums.size(); ++taskIndex)
  taskHandles[taskIndex] = launcher.queueTask(taskFn, "Task " + std::to_string(*(nums.begin() + taskIndex)));

launcher.start();

for (auto& taskHandle : taskHandles)
  std::cout << "Task id: " << taskHandle.id << std::endl << taskHandle.result.get()
```
## Tests
In addition to the library itself, the project contains a demo TestGUI. This program allows you to generate an array of integers and sort its parts, while the parts will be sorted at the same time. The program provides monitoring of the sorting process and the possibility of its interruption. At the end of the procedure, the value range of each sorted part is displayed.

Project build tested on Visual Studio 2019 and gcc 9 (Ubuntu 20.04). To build TestGUI, you need the Qt5 library.
