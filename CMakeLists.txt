cmake_minimum_required(VERSION 3.12...3.16 FATAL_ERROR)

project(TaskQueue)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)
if (NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}")
endif ()
if (NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
endif ()
if (NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
endif ()

add_subdirectory(Library)

option(TEST_GUI "Build tests with gui" ON)
option(TEST_CONSOLE "Build console tests" ON)

if (TEST_GUI OR TEST_CONSOLE)
  add_subdirectory(TestCore)
endif()

if (TEST_GUI)
  add_subdirectory(TestGUI)
endif()


if (TEST_CONSOLE)
  add_subdirectory(TestConsole)
endif()
