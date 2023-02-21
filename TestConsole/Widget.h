#ifndef WIDGET_H
#define WIDGET_H

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

class Widget;
using WidgetChildren = std::vector<std::unique_ptr<Widget>>;
using WidgetIt = WidgetChildren::iterator;
using WidgetCIt = WidgetChildren::const_iterator;
// clang-format off
struct _WidgetInput { enum WidgetInput : char { NO_INPUT, ESC = 27, ENTER = 13, REFRESH = 18, BACK = 8 }; };
// clang-format on
using WidgetInput = _WidgetInput::WidgetInput;

class Widget
{
public:
  static int getInput() noexcept;
  static void ungetInput(int ch) noexcept;
  static bool hasInput() noexcept;
  static std::string inputToStr(int res);

public:
  Widget(const std::string& name);
  virtual ~Widget() = default;

  Widget(const Widget&) = delete;
  Widget(Widget&&) = delete;
  Widget& operator=(const Widget&) = delete;
  Widget& operator=(Widget&&) = delete;

  virtual void draw() = 0;
  virtual int execute() = 0;
  const auto& name() const noexcept { return _name; }

private:
  std::string _name;
};

class ComposedWidget : public Widget
{
public:
  using Widget::Widget;

  void draw() override;
  int execute() override;

protected:
  auto childCount() const noexcept { return _children.size(); };
  auto firstChild() const noexcept { return _children.cbegin(); };
  auto firstChild() noexcept { return _children.begin(); };
  auto endChild() const noexcept { return _children.cend(); };
  auto endChild() noexcept { return _children.end(); };

  auto child(size_t childIndex) const noexcept { return firstChild() + childIndex; };
  auto child(size_t childIndex) noexcept { return firstChild() + childIndex; };
  WidgetCIt child(const std::string& name) const;
  WidgetIt child(const std::string& name);
  template <typename T>
  const T* childWidget(const std::string& name) const;
  template <typename T>
  T* childWidget(const std::string& name);

  WidgetIt pushChild(std::unique_ptr<Widget>&& child);
  template <typename T>
  T* pushChildWidget(std::unique_ptr<Widget>&& child);
  void removeChild(const std::string& name) noexcept { removeChild(child(name)); };
  void removeChild(WidgetCIt child) noexcept { _children.erase(child); };
  void removeChild(size_t childIndex) noexcept { removeChild(child(childIndex)); };

private:
  WidgetChildren _children;
};

template <typename T>
const T* ComposedWidget::childWidget(const std::string& name) const
{
  if (auto child = this->child(name); child != endChild())
    return static_cast<const T*>(child->get());
  else
    return nullptr;
}

template <typename T>
T* ComposedWidget::childWidget(const std::string& name)
{
  if (auto child = this->child(name); child != endChild())
    return static_cast<T*>(child->get());
  else
    return nullptr;
}

template <typename T>
T* ComposedWidget::pushChildWidget(std::unique_ptr<Widget>&& child)
{
  return static_cast<T*>(pushChild(std::move(child))->get());
}

class TableModel;

class TableWidget : public Widget
{
public:
  static constexpr auto colSep = " | ";

public:
  TableWidget(const std::string& name, const std::string& title, TableModel* model);

  void draw() override;
  int execute() override { return WidgetInput::NO_INPUT; }

  void adjustWidth();

private:
  TableModel* _model;
  std::string _title;
  std::vector<uint16_t> _colWidths;
};

class InfoWidget : public Widget
{
public:
  InfoWidget(const std::string& name, const std::string& info);

  void draw() override;
  int execute() override { return WidgetInput::NO_INPUT; }

private:
  std::string _info;
};

class InputWidget : public Widget
{
public:
  using Handler = std::function<void(const std::string&)>;

public:
  InputWidget(const std::string& name, const std::string& title, const std::string& validator, Handler&& handler);

  void draw() override;
  int execute() override;
  const std::string& value() const noexcept { return _value; };

private:
  std::string _title;
  std::string _value;
  std::string _validator;
  Handler _handler;
};

class ActionWidget : public Widget
{
public:
  using Handler = std::function<void(void)>;
  using ActionData = std::pair<std::string, Handler>;
  using Action = std::pair<char, ActionData>;
  using Actions = std::map<Action::first_type, Action::second_type>;

public:
  ActionWidget(const std::string& name, const std::string& title, Actions&& actions);

  void draw() override;
  int execute() override;

private:
  std::string _title;
  const Actions _actions;
};

#endif // WIDGET_H
