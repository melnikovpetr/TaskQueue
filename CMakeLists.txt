cmake_minimum_required(VERSION 3.12...3.16 FATAL_ERROR)

project(TaskQueue)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)

set(SOURCES
  main.cpp
  TaskLauncher.cpp
  TaskQueue.cpp)
source_group(Sources FILES ${SOURCES})

set(HEADERS
  TaskLauncher.h
  TaskQueue.h)
source_group(Headers FILES ${HEADERS})

set(UIS
  )
source_group(UIs FILES ${UIS})

set(EXECUTABLE_FLAGS)

if (WIN32)
    list(APPEND EXECUTABLE_FLAGS WIN32)
endif ()

add_executable(TaskQueue
  ${EXECUTABLE_FLAGS}
  ${SOURCES}
  ${HEADERS}
  ${UIS})

target_link_libraries(TaskQueue
  PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets)
