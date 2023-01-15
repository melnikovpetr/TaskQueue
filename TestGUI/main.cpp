#include "TestDialog.h"

#include <QtWidgets/QApplication>

int main(int argc, char* argv[])
{
  if (getenv("QTDIR"))
    QApplication::addLibraryPath(QString() + getenv("QTDIR") + "/plugins");

  QApplication a{ argc, argv };
  TestDialog dialog{};

  dialog.show();
  a.exec();

  return 0;
}
