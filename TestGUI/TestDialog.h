#ifndef TESTDIALOG_H
#define TESTDIALOG_H

#include <QtWidgets/QDialog>

namespace Ui
{
class TestDialog;
}

class TestDialogInternal;

class TestDialog : public QDialog
{
  Q_OBJECT

public:
  using ArrayValue = int;

public:
  explicit TestDialog(QWidget* parent = nullptr);
  ~TestDialog();

  bool event(QEvent* event) override;

public Q_SLOTS:
  void changeArraySize();
  void resetArraySize();
  void applyArraySize();
  void startStopSorting();

protected:
  std::pair<ArrayValue, ArrayValue> operator()(size_t begin, size_t end) const;

private:
  Ui::TestDialog* _ui;
  TestDialogInternal* _data;
};

#endif // TESTDIALOG_H
