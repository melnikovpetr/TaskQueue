#include "Widget.h"

#include "Keyboard.h"
#include "TableModel.h"

#include <iomanip>
#include <iostream>
#include <regex>
#include <set>
#include <stdexcept>

#define ENDL '\n'

int Widget::getInput() noexcept
{
  Keyboard kb{};
  kb.lineInputOff();
  kb.echoOff();
  return Keyboard::getChar();
}

void Widget::ungetInput(int ch) noexcept
{
  Keyboard kb{};
  kb.lineInputOff();
  kb.echoOff();
  Keyboard::ungetChar(ch);
}

bool Widget::hasInput() noexcept
{
  return Keyboard::isPressed();
}

std::string Widget::inputToStr(int input)
{
  if ((0 <= input) && (input <= 127))
  {
    if (input == WidgetInput::ESC)
      return "ESC";
    if (std::isalnum(input) || std::ispunct(input))
      return std::string(1, input);
  }
  return "";
}

Widget::Widget(const std::string& name)
  : _name{ name }
{
}

void ComposedWidget::draw()
{
  for (const auto& child : _children)
    child->draw();
}

int ComposedWidget::execute()
{
  if (!_children.empty())
    return _children.back()->execute();
  return WidgetInput::NO_INPUT;
}

WidgetCIt ComposedWidget::child(const std::string& name) const
{
  return std::find_if(firstChild(), endChild(), [&name](const WidgetCIt::value_type& child) { return child->name() == name; });
}

WidgetIt ComposedWidget::child(const std::string& name)
{
  return std::find_if(firstChild(), endChild(), [&name](WidgetCIt::value_type& child) { return child->name() == name; });
}

WidgetIt ComposedWidget::pushChild(std::unique_ptr<Widget>&& child)
{
  _children.push_back(std::move(child));
  return this->child(childCount() - 1);
}

TableWidget::TableWidget(const std::string& name, const std::string& title, TableModel* model)
  : Widget(name)
  , _model(model)
  , _colWidths(_model ? _model->colCount() : 0)
{
  adjustWidth();
}

void TableWidget::draw()
{
  if (_model)
  {
    auto colSepLen = std::strlen(colSep);
    auto colTotalWidth = colSepLen;

    // Title
    if (!_title.empty())
      std::cout << _title << ENDL;

    // Header
    std::cout << colSep;
    for (size_t colIndex = 0; colIndex < _model->colCount(); ++colIndex)
    {
      std::cout << std::left << std::setw(_colWidths[colIndex]) << std::setfill(' ') << _model->headerData(colIndex) << colSep;
      colTotalWidth += _colWidths[colIndex] + colSepLen;
    }
    std::cout << ENDL << std::left << std::setw(colTotalWidth) << std::setfill('-') << "" << ENDL;

    // Data
    for (const auto& row : *_model)
    {
      std::cout << colSep;
      for (size_t colIndex = 0; colIndex < _model->colCount(); ++colIndex)
        std::cout << std::left << std::setw(_colWidths[colIndex]) << std::setfill(' ') << row[colIndex] << colSep;
      std::cout << ENDL;
    }
    std::cout << std::left << std::setw(colTotalWidth) << std::setfill('-') << "" << ENDL << ENDL;
  }
}

void TableWidget::adjustWidth()
{
  if (_model)
  {
    _colWidths.resize(_model->colCount());
    for (size_t colIndex = 0; colIndex < _model->colCount(); ++colIndex)
      _colWidths[colIndex] = _model->headerData(colIndex).size();
    for (const auto& row : *_model)
      for (size_t colIndex = 0; colIndex < row.size(); ++colIndex)
        if (_colWidths[colIndex] < row[colIndex].size())
          _colWidths[colIndex] = row[colIndex].size();
  }
}

InputWidget::InputWidget(const std::string& name, const std::string& title, const std::string& validator, Handler&& handler)
  : Widget(name)
  , _title{ title }
  , _value{}
  , _validator{ validator }
  , _handler{ std::move(handler) }
{
}

void InputWidget::draw()
{
  std::cout << _title;
}

int InputWidget::execute()
{
  using Char = decltype(_value)::value_type;
  std::regex re{ _validator };
  std::set<decltype(getInput())> interruptChars{ WidgetInput::ENTER, WidgetInput::ESC, WidgetInput::REFRESH };
  decltype(execute()) input{};

  while (true)
  {
    input = getInput();
    if (interruptChars.count(input))
    {
      std::cout << ENDL;
      if ((input == WidgetInput::ENTER) && _handler)
        _handler(_value);
      break;
    }
    else if (input == WidgetInput::BACK)
    {
      Char ch = input;
      std::cout << ch << ' ' << ch;
      _value.pop_back();
    }
    else if (std::isalnum(input) || std::ispunct(input))
    {
      Char ch = input;
      if (std::regex_match(_value + ch, re))
      {
        std::cout << ch;
        _value += ch;
      }
    }
  }
  return input;
}

InfoWidget::InfoWidget(const std::string& name, const std::string& info)
  : Widget(name)
  , _info{ info }
{
}

void InfoWidget::draw()
{
  std::cout << _info;
}

ActionWidget::ActionWidget(const std::string& name, const std::string& title, Actions&& actions)
  : Widget(name)
  , _title{ title }
  , _actions{ std::move(actions) }
{
}

void ActionWidget::draw()
{
  if (!_title.empty())
    std::cout << _title << ENDL;
  for (const auto& action : _actions)
    if (!action.second.first.empty())
      std::cout << std::right << std::setw(10) << std::setfill(' ') << inputToStr(action.first) << ". " << action.second.first << ENDL;
}

int ActionWidget::execute()
{
  while (true)
  {
    if (auto input = getInput(); input == WidgetInput::REFRESH)
      return input;
    else
    {
      if (auto action = _actions.find(WidgetInput::NO_INPUT); action != _actions.end())
      {
        if (action->second.second)
          action->second.second();
        return WidgetInput::NO_INPUT;
      }
      if (auto action = _actions.find(input); action != _actions.end())
      {
        if (action->second.second)
          action->second.second();
        return input;
      }
    }
  }
}

// auto child = firstChild();
// decltype(execute()) res = WidgetResults::NO_RESULT;
// while ((child = executeAndNext(child, res)) != _children.end())
//  if (res != WidgetResult::NO_RESULT)
//    break;
// return res;

// WidgetIt ComposedWidget::executeAndNext(WidgetIt widgetIt, int& res)
//{
//  res = (*widgetIt)->execute();
//  return ++widgetIt;
//}
