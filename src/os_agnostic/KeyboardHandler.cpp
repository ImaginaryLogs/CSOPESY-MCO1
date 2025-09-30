/**
 * Keyboard handler using OS-dependent Scanner for per-key responsiveness.
 */

#include "KeyboardHandler.hpp"
#include <iostream>

static void printPrompt(MarqueeContext& ctx, const std::string& buf) {
  std::lock_guard<std::mutex> lock(ctx.coutMutex);
  std::cout << "\r\x1b[2K> " << buf << std::flush;
}

void KeyboardHandler::operator()() {
  // >>> JOIN INIT PHASE
  ctx.phase_barrier.arrive_and_wait();

  Scanner scan;
  std::string buffer;
  printPrompt(ctx, buffer);

  while (!ctx.exitRequested.load()) {
    int ch = scan.poll();
    if (ch < 0) continue;

    if (ch == '\n') {
      if (deliver) deliver(buffer);
      buffer.clear();
      printPrompt(ctx, buffer);
    } else if (ch == 3) { // Ctrl+C
      ctx.exitRequested.store(true);
      break;
    } else if (ch == 127 || ch == 8) { // backspace
      if (!buffer.empty()) buffer.pop_back();
      printPrompt(ctx, buffer);
    } else if (ch >= 32 && ch < 127) {
      buffer.push_back(static_cast<char>(ch));
      printPrompt(ctx, buffer);
    }
  }

  // >>> THREAD EXIT
  ctx.stop_latch.count_down();
}
