set(SOURCES
  Keyboard.cpp
  TableModel.cpp
  TestWidget.cpp
  Widget.cpp
  main.cpp)
source_group(Sources FILES ${SOURCES})

set(HEADERS
  Keyboard.h
  TableModel.h
  TestWidget.h
  Widget.h)
source_group(Headers FILES ${HEADERS})

set(EXECUTABLE_FLAGS)

set(PRIVATE_LINK_LIBS
    TaskQueue::TestCore)

add_executable(TestConsole
  ${EXECUTABLE_FLAGS}
  ${SOURCES}
  ${HEADERS})

target_link_libraries(TestConsole
  PRIVATE
    ${PRIVATE_LINK_LIBS})
