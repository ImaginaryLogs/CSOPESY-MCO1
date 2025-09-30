#include "Context.cpp"
#include <optional>
#include <future>
#include "../os_dependent/ScannerLibrary.cpp"
#include <iostream>
#include <string>
#include <chrono>
#include <functional>
#pragma once

class KeyboardHandler : public Handler
{
public:
  KeyboardHandler(MarqueeContext& c) : Handler(c) {}

  void operator()()
  {
    {
      std::lock_guard<std::mutex> lock(ctx.coutMutex);
      std::cout << "... Keyboard Handler is waiting." << std::endl;
    }
    this->ctx.phase_barrier.arrive_and_wait();

    running = true;

    while (running && !ctx.exitRequested.load()) {
      auto ch = CrossPlatformChar::get();

      if (!ch) {
        std::this_thread::sleep_for(slice);
        continue;
      }

      // First keypress of a new input session: show prompt.
      if (!promptShown) {
        showPromptOnNewLine();  // now newline is conditional on marqueeActive
      }

      switch (*ch) {
      case '\r':
      case '\n':
        if (!partialBuffer.empty()) {
          // Clear the prompt line FULLY before executing the command.
          clearPromptLine();
          if (commandHandlerCallback) {
            commandHandlerCallback(partialBuffer);
          }
          partialBuffer.clear();
          lastPromptLen = 0;
        } else {
          // Enter on empty prompt -> clear it to avoid stray '>'
          clearPromptLine();
        }

        // Input session ends. No prompt line now; renderer should not assume one.
        promptShown = false;
        ctx.setHasPromptLine(false);

        // IMPORTANT: do NOT print a newline here — the next keypress will
        // either create a prompt on the same line (no marquee) or below (with marquee).
        break;

      case 3: // Ctrl+C
        ctx.exitRequested.store(true);
        running = false;
        promptShown = false;
        ctx.setHasPromptLine(false);
        break;

      case 8:   // Backspace
      case 127: // DEL (Linux terminals)
        if (!partialBuffer.empty()) {
          partialBuffer.pop_back();
        }
        redrawPrompt(false);
        break;

      default:
        if (*ch >= 32 && *ch < 127) {
          partialBuffer.push_back(static_cast<char>(*ch));
          redrawPrompt(false);
        }
        break;
      }

      std::this_thread::sleep_for(slice);
    }

    ctx.stop_latch.count_down();
  };
  
  void connectHandler(std::function<void(const std::string&)> callbackF){
    std::lock_guard<std::mutex> lock(ctx.coutMutex);
    std::cout << "... Connecting Handler" << std::endl;
    commandHandlerCallback = std::move(callbackF);    
  };

private:
  std::string partialBuffer;
  std::size_t lastPromptLen = 0;
  bool promptShown = false;

  const std::chrono::duration<int64_t, std::milli> slice = std::chrono::milliseconds(30);
  bool running = false;
  std::function<void(const std::string&)> commandHandlerCallback;

  // Show a prompt. If a marquee is running, drop exactly one newline so the prompt
  // is *below* the marquee. If no marquee, reuse current line without extra newline.
  void showPromptOnNewLine()
  {
    std::lock_guard<std::mutex> lock(ctx.coutMutex);

    if (ctx.isMarqueeActive()) {
      std::cout << std::endl;      // go below the marquee line
    } else {
      // Ensure the current line is clean, but do NOT add a newline.
      std::cout << "\r\x1b[2K";
    }

    std::cout << "> " << std::flush;
    promptShown = true;
    lastPromptLen = 0;
    ctx.setHasPromptLine(true);
  }

  // Clear the whole prompt line (no leftover '>'), leave cursor at column 0.
  void clearPromptLine()
  {
#if defined(_WIN32)
    std::lock_guard<std::mutex> lock(ctx.coutMutex);
    std::cout << "\r\x1b[2K" << std::flush;
#else
    std::lock_guard<std::mutex> lock(ctx.coutMutex);
    std::cout << "\r\x1b[2K" << std::flush;
#endif
  }

  // Redraw the prompt WITHOUT wrapping or duplicating arrows.
  void redrawPrompt(bool clearLine)
  {
    std::lock_guard<std::mutex> lock(ctx.coutMutex);

    const std::size_t curLen = clearLine ? 0 : partialBuffer.size();
    std::size_t pad = 0;
    if (lastPromptLen > curLen) pad = lastPromptLen - curLen;
    lastPromptLen = curLen;

    if (clearLine) {
      std::cout << "\r\x1b[2K" << std::flush;
      return;
    }

    std::cout << "\r> " << partialBuffer
              << std::string(pad, ' ')
              << "\r> " << partialBuffer
              << std::flush;
  }
};