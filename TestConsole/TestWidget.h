#ifndef TEST_WIDGET_H
#define TEST_WIDGET_H

#include "Widget.h"

#include "TableModel.h"

#include <ArraySort.h>

class MainWidget : public ComposedWidget
{
  // clang-format off
  enum : size_t { TLI_ID, TLI_THREAD_ID, TLI_INFO, TLI_PROGRESS, TLI_RESULT, _TLI_COUNT };
  enum : size_t { SI_THREAD_POOL, SI_ARRAY, _SI_COUNT };
  // clang-format on
public:
  MainWidget();

  void draw() override;

protected:
  std::unique_ptr<Widget> createWidget(const std::string& name);

  void readArraySize();
  void startStop();
  void setArraySize(const std::string& value);

private:
  TableModel _taskLauncherInfo;
  TableModel _sizesInfo;
  ArraySort _arraySort;
};

#endif // WIDGET_H
