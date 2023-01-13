set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(SOURCES
  TaskLauncher.cpp
  TaskQueue.cpp)
source_group(Sources FILES ${SOURCES})

set(HEADERS
  TaskQueue.h
  TaskLauncher.h)
source_group(Headers FILES ${HEADERS})

set(PRIVATE_LINK_LIBS
  )

if (NOT MSVC)
  list (APPEND PRIVATE_LINK_LIBS
    pthread)
endif ()

add_library(TaskQueue SHARED)
add_library(TaskQueue::TaskQueue ALIAS TaskQueue)

include(GenerateExportHeader)
set(TARGET_EXPORT_HEADER TaskQueueExport.h)
generate_export_header(TaskQueue
  EXPORT_MACRO_NAME TASKQUEUE_EXPORT
  EXPORT_FILE_NAME ${TARGET_EXPORT_HEADER})

target_include_directories(TaskQueue
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

target_sources(TaskQueue
  PRIVATE
    ${HEADERS}
    ${SOURCES})

target_link_libraries(TaskQueue
  PRIVATE
    ${PRIVATE_LINK_LIBS})

if(NOT CMAKE_SKIP_INSTALL_RULES)
  install(FILES ${headers} ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_EXPORT_HEADER}
    DESTINATION include)
endif()
