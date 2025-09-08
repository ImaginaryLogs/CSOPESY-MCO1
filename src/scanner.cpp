#include "contract.cpp"
#include <optional>
#include <future>
#pragma once

#pragma once
#include <optional>
#include <chrono>

#if defined(_WIN32)
#include <conio.h>
#include <windows.h>
#else
#include <unistd.h>
#include <termios.h>
#include <sys/select.h> // for select()
#endif

class CrossPlatformChar
{
public:
  static std::optional<char> readCharFor(std::chrono::milliseconds timeout)
  {
#if defined(_WIN32)
    auto end = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < end)
    {
      if (_kbhit())
      {
        return static_cast<char>(_getch());
      }
      Sleep(1);
    }
    return std::nullopt;

#else // POSIX
    enableRaw(); // Set terminal to raw mode to not interpret special characters
    // https://en.wikipedia.org/wiki/Terminal_mode 
    
    // Get File Descriptor Set, Input is from FDS_SET = 0
    // Essentially, get the actual raw sacanner
    fd_set fds; 
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);

    // timing out after the specified duration
    struct timeval tv;
    tv.tv_sec = timeout.count() / 1000;
    tv.tv_usec = (timeout.count() % 1000) * 1000;

    std::optional<char> out;
    // wait for input or timeout
    int r = select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv); 

    // https://stackoverflow.com/questions/29307390/how-does-fd-isset-work
    if (r > 0 && FD_ISSET(STDIN_FILENO, &fds))
    {
      char c;

      // Read a single character
      if (::read(STDIN_FILENO, &c, 1) > 0)
        out = c;
    }

    disableRaw();
    return out;
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
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
  }
  static void disableRaw()
  {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig);
  }
#endif
};

class InputScanner
{
public:
  explicit InputScanner(SyncContext &c) : ctx(c) {}

  void operator()()
  {
    using clock = std::chrono::steady_clock;
    const auto slice = std::chrono::milliseconds(50); // input gets 0.05s

    while (true)
    {
      std::unique_lock<std::mutex> lock(ctx.screenLock);
      ctx.cv.wait(lock, scannerTurnPredicate);
      if (!ctx.running)
        break;

      auto deadline = clock::now() + slice;
      while (clock::now() < deadline && ctx.running)
      {
        lock.unlock();

        std::cout << "\rInput: " << partialBuffer << std::flush;
        auto ch = CrossPlatformChar::readCharFor(slice);
        // std::cout << "\rInput: " << partialBuffer << std::flush;
        if (ch)
        {
          if (*ch == '\r' || *ch == '\n')
          {

            partialBuffer.clear();
            break; // done for this slice
          }
          else if (*ch == 127)
          {
            if (!partialBuffer.empty())
            {
              partialBuffer.pop_back();
            }
          }
          else
          {
            partialBuffer.push_back(*ch);
          }
        }

        lock.lock();
        if (!ctx.running)
          break;
      }

      ctx.turn = SyncContext::Turn::Display;
      lock.unlock();
      ctx.cv.notify_all();
    }
  };
  void processChoice();

private:
  SyncContext &ctx;
  std::string partialBuffer;

  bool running = false;
  std::string getUserInput();

  bool isScannerTurn() { return ctx.turn == SyncContext::Turn::Input || !ctx.running; };
  std::function<bool()> scannerTurnPredicate = [this]()
  { return isScannerTurn(); };
};
