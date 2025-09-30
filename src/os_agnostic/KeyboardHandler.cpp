/**
 * Keyboard handler using OS-dependent Scanner for per-key responsiveness.
 */

#include "KeyboardHandler.hpp"
#include <iostream>

static void redrawPrompt(MarqueeContext& ctx, const std::string& buf) {
  // Always redraw the prompt at the saved prompt anchor.
  std::lock_guard<std::mutex> lock(ctx.coutMutex);
  // Restore to prompt anchor (safe even if already there)
  std::cout << "\x1b[u";
  // Clear the prompt line and print prompt + buffer
  std::cout << "\r\x1b[2K> " << buf;
  // Save cursor at the end of the prompt line (anchor for everyone)
  std::cout << "\x1b[s" << std::flush;

  ctx.setHasPromptLine(true);
}

void KeyboardHandler::operator()() {
  // >>> JOIN INIT PHASE
  ctx.phase_barrier.arrive_and_wait();

  Scanner scan;
  std::string buffer;

  // Create a *dedicated marquee line* above the prompt, then draw the prompt and save anchor.
  {
    std::lock_guard<std::mutex> lock(ctx.coutMutex);
    std::cout << "\n";      // this is the marquee line
    std::cout << "> ";      // print prompt for the first time
    std::cout << "\x1b[s";  // save cursor as the prompt anchor
    std::cout << std::flush;
  }
  ctx.setHasPromptLine(true);

  while (!ctx.exitRequested.load()) {
    int ch = scan.poll();
    if (ch < 0) continue;

    if (ch == '\n') {                     // Enter: submit
      if (deliver) deliver(buffer);
      buffer.clear();
      redrawPrompt(ctx, buffer);
    } else if (ch == 3) {                 // Ctrl+C: exit
      ctx.exitRequested.store(true);
      break;
    } else if (ch == 127 || ch == 8) {    // backspace
      if (!buffer.empty()) buffer.pop_back();
      redrawPrompt(ctx, buffer);
    } else if (ch >= 32 && ch < 127) {    // visible ASCII
      buffer.push_back(static_cast<char>(ch));
      redrawPrompt(ctx, buffer);
    }
  }

  // Clear prompt on exit
  {
    std::lock_guard<std::mutex> lock(ctx.coutMutex);
    std::cout << "\x1b[u"     // to prompt anchor
              << "\r\x1b[2K"  // clear prompt line
              << std::flush;
  }
  ctx.setHasPromptLine(false);

  // >>> THREAD EXIT
  ctx.stop_latch.count_down();
}
