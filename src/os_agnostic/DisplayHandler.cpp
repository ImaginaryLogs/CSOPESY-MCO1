/**
 * Renders the marquee frames to the console.
 */

#include "DisplayHandler.hpp"
#include <chrono>
#include <thread>
#include <iostream>

#if defined(_WIN32)
#include <windows.h>
static void enableVirtualTerminal() {
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  if (hOut == INVALID_HANDLE_VALUE) return;
  DWORD dwMode = 0;
  if (!GetConsoleMode(hOut, &dwMode)) return;
  dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
  SetConsoleMode(hOut, dwMode);
}
#else
static void enableVirtualTerminal() {}
#endif

std::string DisplayHandler::scrollOnce(const std::string& s) {
  if (s.empty()) return s;
  std::string out = s.substr(1) + s.substr(0,1);
  return out;
}

void DisplayHandler::operator()() {
  // >>> JOIN INIT PHASE
  ctx.phase_barrier.arrive_and_wait();

  enableVirtualTerminal();

  std::string cur = ctx.getText();

  while (!ctx.exitRequested.load()) {
    if (ctx.isMarqueeActive()) {
      {
        std::lock_guard<std::mutex> guard(ctx.textMutex);
        cur = ctx.marqueeText;
        if (!cur.empty()) {
          ctx.marqueeText = scrollOnce(cur);
          cur = ctx.marqueeText;
        }
      }

      {
        std::lock_guard<std::mutex> lock(ctx.coutMutex);
        if (ctx.getHasPromptLine()) {
          // Draw on the line ABOVE the prompt, then restore to the prompt anchor.
          std::cout << "\x1b[u"      // restore to prompt anchor
                    << "\x1b[1F"     // move to line above (marquee line)
                    << "\r\x1b[2K"   // clear marquee line
                    << cur
                    << "\x1b[u"      // restore to prompt anchor
                    << std::flush;
        } else {
          // No prompt/prompt-anchor yet: draw on current line
          std::cout << "\r\x1b[2K" << cur << std::flush;
        }
      }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(ctx.speedMs.load()));
  }

  // >>> THREAD EXIT
  ctx.stop_latch.count_down();
}
