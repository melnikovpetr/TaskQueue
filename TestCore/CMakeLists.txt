set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(SOURCES
  ArraySort.cpp)
source_group(Sources FILES ${SOURCES})

set(HEADERS
  ArraySort.h)
source_group(Headers FILES ${HEADERS})

set(PUBLIC_LINK_LIBS
  TaskQueue::TaskQueue)

add_library(TestCore STATIC)
add_library(TaskQueue::TestCore ALIAS TestCore)

target_include_directories(TestCore
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

target_sources(TestCore
  PRIVATE
    ${HEADERS}
    ${SOURCES})

target_link_libraries(TestCore
  PUBLIC
    ${PUBLIC_LINK_LIBS})

if(NOT CMAKE_SKIP_INSTALL_RULES)
  install(FILES ${headers}
    DESTINATION include)
endif()
