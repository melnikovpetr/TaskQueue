#ifndef TESTDIALOG_H
#define TESTDIALOG_H

#include <QtWidgets/QDialog>

namespace Ui
{
class TestDialog;
}

class TestDialogInternal;
using ArrayValue = int;

class TestDialog : public QDialog
{
  Q_OBJECT

public:
  explicit TestDialog(QWidget* parent = nullptr);
  ~TestDialog();

  bool event(QEvent* event) override;

public Q_SLOTS:
  void changeArraySize();
  void applyArraySize();
  void startStopSorting(bool start);

private:
  Ui::TestDialog* _ui;
  TestDialogInternal* _data;
};

#endif // TESTDIALOG_H
