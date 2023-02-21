#include "TestWidget.h"

#include <iostream>

int main(int argc, char* argv[])
{
  std::ios::sync_with_stdio(false);

  MainWidget mainWidget{};

  while (true)
  {
    mainWidget.draw();
    if (mainWidget.execute() == WidgetInput::ESC)
      break;
  }

  return 0;
}
