#include "Keyboard.h"

#include <cstdio>
#include <iostream>

void Keyboard::clearConsole()
{
  std::cout << "\033[2J\033[1;1H";
}

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

static bool setKeyModeFlag(HANDLE inputHandle, DWORD& mode, DWORD flag, bool on)
{
  auto changed = on ? !(mode & flag) : (mode & flag);
  if (changed)
    ::SetConsoleMode(inputHandle, on ? (mode = mode | flag) : (mode = mode & ~flag));
  return changed;
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

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

int Keyboard::getChar() noexcept
{
  Keyboard kb{};
  kb.echoOff();
  kb.lineInputOff();
  while (true)
  {
    if (auto ch = std::getc(stdin); ch != EOF)
      return ch;
    usleep(100000);
  }
}

void Keyboard::ungetChar(int ch) noexcept
{
  std::ungetc(ch, stdin);
}

bool Keyboard::isPressed() noexcept
{
  Keyboard kb{};
  kb.lineInputOff();
  if (auto ch = std::getc(stdin); ch != EOF)
  {
    std::ungetc(ch, stdin);
    return true;
  }
  return false;
}

struct KeyboardMode
{
  KeyboardMode()
    : inputHandle{ STDIN_FILENO }
  {
    fcntlOld = fcntlCurr = ::fcntl(STDIN_FILENO, F_GETFL, 0);
    ::tcgetattr(inputHandle, &termiosCurr);
    c_lflagOld = termiosCurr.c_lflag;
  }

  void restore() noexcept
  {
    if (termiosCurr.c_lflag != c_lflagOld)
      ::tcsetattr(inputHandle, TCSANOW, (termiosCurr.c_lflag = c_lflagOld, &termiosCurr));
    if (fcntlCurr != fcntlOld)
      ::fcntl(inputHandle, F_SETFL, (fcntlCurr = fcntlOld));
  }

  int inputHandle;
  int fcntlOld;
  int fcntlCurr;
  tcflag_t c_lflagOld;
  termios termiosCurr;
};

Keyboard::Keyboard()
  : _mode{ std::make_unique<KeyboardMode>() }
{
}

Keyboard::~Keyboard()
{
  _mode->restore();
}

void Keyboard::echoOff() noexcept
{
  _mode->termiosCurr.c_lflag &= ~ECHO;
  ::tcsetattr(_mode->inputHandle, TCSANOW, &_mode->termiosCurr);
}

void Keyboard::echoOn() noexcept
{
  _mode->termiosCurr.c_lflag |= ECHO;
  ::tcsetattr(_mode->inputHandle, TCSANOW, &_mode->termiosCurr);
}

void Keyboard::lineInputOff() noexcept
{
  _mode->fcntlCurr |= O_NONBLOCK;
  ::fcntl(_mode->inputHandle, F_SETFL, _mode->fcntlCurr);
  _mode->termiosCurr.c_lflag &= ~ICANON;
  ::tcsetattr(_mode->inputHandle, TCSANOW, &_mode->termiosCurr);
}

void Keyboard::lineInputOn() noexcept
{
  _mode->fcntlCurr &= ~O_NONBLOCK;
  ::fcntl(_mode->inputHandle, F_SETFL, _mode->fcntlCurr);
  _mode->termiosCurr.c_lflag |= ICANON;
  ::tcsetattr(_mode->inputHandle, TCSANOW, &_mode->termiosCurr);
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
