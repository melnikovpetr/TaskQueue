cmake_minimum_required(VERSION 3.12...3.16 FATAL_ERROR)

project(TaskQueue)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(Library)

set(TEST_GUI ON)
if (TEST_GUI)
  add_subdirectory(TestGUI)
endif()

set(TEST_CONSOLE ON)
if (TEST_CONSOLE)
  add_subdirectory(TestConsole)
endif()
