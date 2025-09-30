#include <string>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include "Context.cpp"
#include <functional>
#include <chrono>
#include <thread>
#include <algorithm>

#if defined(_WIN32)
#include <windows.h>
#endif

class DisplayHandler : public Handler
{
public:
  DisplayHandler(MarqueeContext& c) : Handler(c) {}

  void operator()()
  {
    {
      std::lock_guard<std::mutex> lock(ctx.coutMutex);
      std::cout << "... Display Handler is waiting." << std::endl;
    }
    this->ctx.phase_barrier.arrive_and_wait();

#if defined(_WIN32)
    enableVirtualTerminal(); // allow ANSI save/restore cursor on Windows 10+
#endif

    std::size_t offset = 0;
    const std::string gap = "    ";

    while (!ctx.exitRequested.load()) {
      if (!isVideoRunning.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        continue;
      }

      std::string text;
      {
        std::lock_guard<std::mutex> lock(ctx.textMutex);
        text = currentFrame.empty() ? ctx.marqueeText : currentFrame;
      }
      if (text.empty()) text = " ";

      const std::size_t frameWidth = text.size() + gap.size();
      const std::string pad(frameWidth, ' ');
      const std::string displayBuffer = pad + text + pad;
      const std::size_t maxOffset = displayBuffer.size() - frameWidth;

      if (offset > maxOffset) offset = 0;
      const std::string frame = displayBuffer.substr(offset, frameWidth);

      {
        std::lock_guard<std::mutex> lock(ctx.coutMutex);

        // If a prompt line exists, draw the marquee one line ABOVE it.
        if (ctx.getHasPromptLine()) {
#if defined(_WIN32)
          std::cout << "\x1b[s"              // save cursor (prompt)
                    << "\x1b[1F"             // move to previous line (marquee line)
                    << "\r\x1b[2K"           // full erase
                    << "Video Thread: " << frame
                    << "\x1b[u"              // restore to prompt
                    << std::flush;
#else
          std::cout << "\x1b[s" << "\x1b[1F" << "\r\x1b[2K"
                    << "Video Thread: " << frame
                    << "\x1b[u" << std::flush;
#endif
        } else {
#if defined(_WIN32)
          std::cout << "\r\x1b[2K"           // redraw on the current line
                    << "Video Thread: " << frame
                    << std::flush;
#else
          std::cout << "\r\x1b[2K"
                    << "Video Thread: " << frame
                    << std::flush;
#endif
        }
      }

      offset++;
      std::this_thread::sleep_for(std::chrono::milliseconds(ctx.speedMs.load()));
    }

    ctx.stop_latch.count_down();
  };

  void clearScreen()
  {
    std::lock_guard<std::mutex> lock(ctx.coutMutex);
#if defined(_WIN32)
    std::cout << "\x1b[2J\x1b[1;1H" << std::flush;
#else
    std::cout << "\x1b[2J\x1b[1;1H" << std::flush;
#endif
  };

  void ping()
  {
    std::lock_guard<std::mutex> lock(ctx.coutMutex);
    std::cout << "Display Handler received ping..." << std::endl;
  }

  // Update text only; render loop handles drawing. No printing here.
  void displayString(const std::string &input)
  {
    std::lock_guard<std::mutex> lock(ctx.textMutex);
    currentFrame = input;
  }

  void start()
  {
    isVideoRunning.store(true);
    ctx.setMarqueeActive(true);    // <-- insertion: tell keyboard we have a marquee line
  }

  void stop()
  {
    // Fully clear the marquee line (whether prompt exists or not), then stop.
    std::lock_guard<std::mutex> lock(ctx.coutMutex);
    if (ctx.getHasPromptLine()) {
#if defined(_WIN32)
      std::cout << "\x1b[s" << "\x1b[1F" << "\r\x1b[2K" << "\x1b[u" << std::flush;
#else
      std::cout << "\x1b[s" << "\x1b[1F" << "\r\x1b[2K" << "\x1b[u" << std::flush;
#endif
    } else {
#if defined(_WIN32)
      std::cout << "\r\x1b[2K" << std::flush;
#else
      std::cout << "\r\x1b[2K" << std::flush;
#endif
    }
    isVideoRunning.store(false);
    ctx.setMarqueeActive(false);   // <-- insertion: keyboard should not add a newline now
  }

private:
  std::string currentFrame;
  std::atomic<bool> isVideoRunning{false};

#if defined(_WIN32)
  void enableVirtualTerminal()
  {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;
    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) return;
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
  }
#endif
};