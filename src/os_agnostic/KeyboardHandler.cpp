#include "Context.cpp"
#include <optional>
#include <future>
#include "os_depedent/ScannerLibrary.cpp"
#include <iostream>
#include <string>
#include <chrono>
#pragma once

class InputScanner
{
public:
  explicit InputScanner(MarqueeContext &c) : ctx(c) {}

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
            // done for this slice
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

      ctx.turn = MarqueeContext::Turn::Display;
      lock.unlock();
      ctx.cv.notify_all();
    }
  };
  void processChoice();

private:
  MarqueeContext &ctx;
  std::string partialBuffer;

  bool running = false;
  std::string getUserInput();

  bool isScannerTurn() { return ctx.turn == MarqueeContext::Turn::Input || !ctx.running; };
  std::function<bool()> scannerTurnPredicate = [this]()
  { return isScannerTurn(); };
};
