/**
 * Keyboard handler using OS-dependent Scanner for per-key responsiveness.
 */

#include "KeyboardHandler.hpp"
#include <iostream>

static void ensurePromptAnchor(MarqueeContext& ctx) {
  // If we don't have a prompt yet, create two lines for [status][marquee], then the prompt.
  if (!ctx.getHasPromptLine()) {
    std::lock_guard<std::mutex> lock(ctx.coutMutex);
    std::cout << "\n\n"     // allocate [status] + [marquee] lines
              << "> "       // print prompt
              << "\x1b[s"   // save anchor at end of prompt
              << std::flush;
    ctx.setHasPromptLine(true);
  }
}

static void redrawPrompt(MarqueeContext& ctx, const std::string& buf) {
  // Always redraw at the saved prompt anchor (one line below marquee).
  std::lock_guard<std::mutex> lock(ctx.coutMutex);
  std::cout << "\x1b[u"         // restore to prompt anchor
            << "\r\x1b[2K> "    // clear prompt line + print "> "
            << buf              // typed buffer
            << "\x1b[s"         // re-save anchor at end-of-line
            << std::flush;
  ctx.setHasPromptLine(true);
}

void KeyboardHandler::operator()() {
  // >>> JOIN INIT PHASE
  ctx.phase_barrier.arrive_and_wait();

  Scanner scan;
  std::string buffer;

  // Establish the 3-line layout and anchor once.
  ensurePromptAnchor(ctx);

  while (!ctx.exitRequested.load()) {
    if (!ctx.getHasPromptLine()) {
      ensurePromptAnchor(ctx);
    }

    int ch = scan.poll();
    if (ch < 0) continue;

    if (ch == '\n') {
      // 1) Command text disappears now: clear prompt and show fresh "> " immediately.
      const std::string submitted = buffer;
      buffer.clear();
      // redrawPrompt(ctx, buffer);

      // 2) Then deliver the command (feedback will print above; marquee will render below).
      if (deliver) deliver(submitted);

    } else if (ch == 3) { // Ctrl+C
      ctx.exitRequested.store(true);
      break;

    } else if (ch == 127 || ch == 8) { // backspace
      if (!buffer.empty()) buffer.pop_back();
      redrawPrompt(ctx, buffer);

    } else if (ch >= 32 && ch < 127) { // printable ASCII
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
