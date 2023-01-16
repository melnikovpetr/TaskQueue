#include <TaskLauncher.h>

#include <chrono>
#include <iostream>
#include <sstream>

using namespace std::chrono_literals;

int main(int argc, char* argv[])
{
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
  /*std::result_of<decltype(taskFn), const std::string&>::*/
  for (decltype(nums.size()) taskIndex = 0; taskIndex < nums.size(); ++taskIndex)
    taskHandles[taskIndex] = launcher.queueTask(taskFn, "Task " + std::to_string(*(nums.begin() + taskIndex)));

  std::cout << "Press 'Y' to start threads: ";
  if (std::getchar() == 'Y')
  {
    launcher.start();
    for (auto& taskHandle : taskHandles)
      std::cout << "Task id: " << taskHandle.id << std::endl << taskHandle.result.get() << std::endl << std::endl;
  }

  return 0;
}
