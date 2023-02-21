#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <memory>

struct KeyboardMode;

class Keyboard
{
public:
  static void clearConsole();
  static int getChar() noexcept;
  static void ungetChar(int ch) noexcept;
  static bool isPressed() noexcept;

public:
  Keyboard();
  ~Keyboard();

  void echoOff() noexcept;
  void echoOn() noexcept;

  void lineInputOff() noexcept;
  void lineInputOn() noexcept;

private:
  std::unique_ptr<KeyboardMode> _mode;
};

#endif // KEYBOARD_H
