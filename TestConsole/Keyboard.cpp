#include "Keyboard.h"

#include <cstdio>
#include <iostream>

#ifdef _WIN32

#include <SpinMutex.h>

#include <mutex>

#include <Windows.h>
#include <conio.h>

static SpinMutex& ioSpinLock()
{
  static SpinMutex _ioSpinLock{};
  return _ioSpinLock;
}

void Keyboard::clearConsole()
{
  system("cls");
}

int Keyboard::getChar() noexcept
{
  while (true)
  {
    {
      std::unique_lock spinLock{ ioSpinLock() };
      if (::_kbhit() > 0)
        return ::_getch_nolock();
    }
    Sleep(100);
  }
}

void Keyboard::ungetChar(int ch) noexcept
{
  std::unique_lock spinLock{ ioSpinLock() };
  ::_ungetch_nolock(ch);
}

bool Keyboard::isPressed() noexcept
{
  std::unique_lock spinLock{ ioSpinLock() };
  return ::_kbhit() > 0;
}

static bool setKeyModeFlag(HANDLE inputHandle, DWORD& mode, DWORD flag, bool on)
{
  auto changed = on ? !(mode & flag) : (mode & flag);
  if (changed)
    ::SetConsoleMode(inputHandle, on ? (mode = mode | flag) : (mode = mode & ~flag));
  return changed;
}

struct KeyboardMode
{
  KeyboardMode()
    : inputHandle{ ::GetStdHandle(STD_INPUT_HANDLE) }
    , old{ 0 }
    , curr{ 0 }
  {
    ::GetConsoleMode(inputHandle, &old);
    curr = old;
  }

  HANDLE inputHandle;
  DWORD old;
  DWORD curr;
};

Keyboard::Keyboard()
  : _mode{ std::make_unique<KeyboardMode>() }
{
}

Keyboard::~Keyboard()
{
  if (_mode->curr != _mode->old)
    ::SetConsoleMode(_mode->inputHandle, _mode->old);
}

void Keyboard::echoOff() noexcept
{
  ::setKeyModeFlag(_mode->inputHandle, _mode->curr, ENABLE_ECHO_INPUT, false);
}

void Keyboard::echoOn() noexcept
{
  ::setKeyModeFlag(_mode->inputHandle, _mode->curr, ENABLE_ECHO_INPUT, true);
}

void Keyboard::lineInputOff() noexcept
{
  ::setKeyModeFlag(_mode->inputHandle, _mode->curr, ENABLE_LINE_INPUT, false);
}

void Keyboard::lineInputOn() noexcept
{
  ::setKeyModeFlag(_mode->inputHandle, _mode->curr, ENABLE_LINE_INPUT, false);
}

#else

#include <sys/ioctl.h>
#include <termios.h>

bool isKeyPressed()
{
  static const int STDIN{ 0 };

  termios term{};
  int bytesWaiting{ 0 };
  tcgetattr(STDIN, &term);

  term.c_lflag &= ~ICANON;
  tcsetattr(STDIN, TCSANOW, &term);

  ioctl(0, FIONREAD, &bytesWaiting);

  term.c_lflag |= ICANON;
  tcsetattr(STDIN, TCSANOW, &term);

  return bytesWaiting > 0;
}

void turnEchoOff()
{
  static const int STDIN{ 0 };
  termios term{};
  tcgetattr(STDIN, &term);
  term.c_lflag &= ~ECHO;
  tcsetattr(STDIN, TCSANOW, &term);
}

void turnEchoOn()
{
  static const int STDIN{ 0 };
  termios term{};
  tcgetattr(STDIN, &term);
  term.c_lflag |= ECHO;
  tcsetattr(STDIN, TCSANOW, &term);
}

#endif // _WIN32

//#include <condition_variable>
//
// static std::condition_variable_any& ioCondVar()
//{
//  static std::condition_variable_any _ioCondVar{};
//  return _ioCondVar;
//}
//
// int Keyboard::getChar() noexcept
//{
//  std::unique_lock spinLock{ ioSpinLock() };
//  ioCondVar().wait(spinLock, []() noexcept { return ::_kbhit() > 0; });
//  return ::_getch_nolock();
//}
//
// void Keyboard::ungetChar(int ch) noexcept
//{
//  {
//    std::unique_lock spinLock{ ioSpinLock() };
//    ::_ungetch_nolock(ch);
//  }
//  ioCondVar().notify_one();
//}