#if defined(_WIN32)
#include <conio.h>
#include <windows.h>
#else
#include <unistd.h>
#include <termios.h>
#include <sys/select.h> // for select()
#include <sys/ioctl.h>
#endif


#include <optional>
#include <chrono>

#pragma once
class CrossPlatformChar
{
public:
  // Non-blocking single character fetch.
  static std::optional<int> get()
  {
#ifdef _WIN32
    if (_kbhit())
    {
      int ch = _getch();
      return ch;
    }
    return std::nullopt;
#else
    // Put terminal in raw, non-canonical, no-echo mode
    enableRaw();
    // Poll stdin with select for immediate availability
    fd_set set;
    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);
    timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0; // non-blocking

    int rv = select(STDIN_FILENO + 1, &set, nullptr, nullptr, &tv);
    int out = -1;

    if (rv > 0 && FD_ISSET(STDIN_FILENO, &set)) {
      unsigned char c;
      ssize_t n = read(STDIN_FILENO, &c, 1);
      if (n == 1) out = static_cast<int>(c);
    }

    disableRaw();
    if (out >= 0) return out;
    return std::nullopt;
    
#endif
  }

private:
#ifndef _WIN32
  static inline termios orig{};

  static void enableRaw()
  {
    tcgetattr(STDIN_FILENO, &orig);
    termios raw = orig;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
  }

  static void disableRaw()
  {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig);
  }
#endif
};
