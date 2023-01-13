set(SOURCES
  main.cpp)
source_group(Sources FILES ${SOURCES})

set(HEADERS
  )
source_group(Headers FILES ${HEADERS})

set(EXECUTABLE_FLAGS)

set(PRIVATE_LINK_LIBS
  TaskQueue)

add_executable(TestConsole
  ${EXECUTABLE_FLAGS}
  ${SOURCES}
  ${HEADERS})

target_link_libraries(TestConsole
  PRIVATE
    ${PRIVATE_LINK_LIBS})
