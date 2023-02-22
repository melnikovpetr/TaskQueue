#include "TestWidget.h"

#include <iostream>

int main(int argc, char* argv[])
{
  TestWidget testWidget{};

  while (true)
  {
    testWidget.draw();
    if (testWidget.execute() == WidgetInput::ESC)
      break;
  }

  return 0;
}
