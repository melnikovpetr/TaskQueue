set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(QT NAMES Qt5)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(SOURCES
  main.cpp)
source_group(Sources FILES ${SOURCES})

set(HEADERS
  )
source_group(Headers FILES ${HEADERS})

set(UIS
  )
source_group(UIs FILES ${UIS})

set(EXECUTABLE_FLAGS)
if (WIN32)
    list(APPEND EXECUTABLE_FLAGS WIN32)
endif ()

set(PRIVATE_LINK_LIBS
  Qt${QT_VERSION_MAJOR}::Widgets
  TaskQueue::TaskQueue)

add_executable(TestGUI
  ${EXECUTABLE_FLAGS}
  ${SOURCES}
  ${HEADERS}
  ${UIS})

target_link_libraries(TestGUI
  PRIVATE
    ${PRIVATE_LINK_LIBS})
