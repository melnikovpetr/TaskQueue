#include <QtWidgets/QApplication>

int main(int argc, char* argv[])
{
  if (getenv("QTDIR"))
    QApplication::addLibraryPath(QString() + getenv("QTDIR") + "/plugins");

  QApplication a{ argc, argv };
  a.exec();
  return 0;
}
