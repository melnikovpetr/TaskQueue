set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(SOURCES
  TaskLauncher.cpp
  TaskQueue.cpp)

set(PUBLIC_HEADERS
  TaskLauncher.h)

set(PRIVATE_HEADERS
  TaskQueue.h)

set(PRIVATE_LINK_LIBS
  )

if (NOT MSVC)
  list (APPEND PRIVATE_LINK_LIBS
    pthread)
endif ()

add_library(TaskQueue SHARED)
add_library(TaskQueue::TaskQueue ALIAS TaskQueue)

include(GenerateExportHeader)
set(TASKQUEUE_EXPORT_FILE TaskQueueExport.h)
generate_export_header(TaskQueue
  EXPORT_MACRO_NAME TASKQUEUE_EXPORT
  EXPORT_FILE_NAME ${TASKQUEUE_EXPORT_FILE})
list (APPEND PUBLIC_HEADERS
  ${CMAKE_CURRENT_BINARY_DIR}/${TASKQUEUE_EXPORT_FILE})

target_sources(TaskQueue
  PUBLIC
    ${PUBLIC_HEADERS}
  PRIVATE
    ${PRIVATE_HEADERS}
    ${SOURCES})

set_target_properties(TaskQueue PROPERTIES
    PUBLIC_HEADER "${PUBLIC_HEADERS}")

target_link_libraries(TaskQueue
  PRIVATE
    ${PRIVATE_LINK_LIBS})

install(FILES ${headers} ${CMAKE_CURRENT_BINARY_DIR}/${TASKQUEUE_EXPORT_FILE}
  DESTINATION include)
