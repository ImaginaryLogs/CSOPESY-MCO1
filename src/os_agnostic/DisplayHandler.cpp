/**
 * Renders the marquee frames to the console.
 */

#include "DisplayHandler.hpp"
#include <chrono>
#include <thread>
#include <iostream>
#include <algorithm>   // <-- needed for (std::max)

#if defined(_WIN32)
#define NOMINMAX       // <-- prevent Windows macros min/max
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
          // Viewport
          int H = (std::max)(1, ctx.getFrameHeight());
          int W = (std::max)(1, ctx.getFrameWidth());

          // Build exactly W characters for this frame
          std::string view = cur;
          if ((int)view.size() < W) view.append(W - (int)view.size(), ' ');
          else if ((int)view.size() > W) view.erase(W);

          // Choose center line: odd -> middle; even -> higher middle
          int centerTop = (H % 2 == 1) ? (H / 2) : (H / 2 - 1);  // 0-based from top
          int drawK = H - centerTop;                             // 1-based from bottom (kF uses bottom=1F)

          // Paint H lines above the prompt (no newlines; we repaint in-place)
          for (int k = 1; k <= H; ++k) {
            std::cout << "\x1b[u"                // back to prompt anchor
                      << "\x1b[" << k << "F"     // move up k lines (1F = bottom-most line)
                      << "\r\x1b[2K";            // clear that line
            if (k == drawK) std::cout << view;   // draw marquee on the centered line only
          }
          std::cout << "\x1b[u" << std::flush;   // restore to prompt anchor
        } else {
          // No prompt/prompt-anchor yet: draw on current line (fallback)
          std::cout << "\r\x1b[2K" << cur << std::flush;
        }
      }

    }

    std::this_thread::sleep_for(std::chrono::milliseconds(ctx.speedMs.load()));
  }

  // >>> THREAD EXIT
  ctx.stop_latch.count_down();
}
