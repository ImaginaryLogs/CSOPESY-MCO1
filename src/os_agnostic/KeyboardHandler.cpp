#include "Context.cpp"
#include <optional>
#include <future>
#include "../os_dependent/ScannerLibrary.cpp"
#include <iostream>
#include <string>
#include <chrono>
#include <functional>
#include <atomic>

#pragma once

// Reads keystrokes interactively and forwards complete lines to CommandHandler.
class KeyboardHandler : public Handler
{
public:
  explicit KeyboardHandler(MarqueeContext& c) : Handler(c) {}

  void operator()()
  {
    std::cout << "... Keyboard Handler is waiting.\n";
    ctx.phase_barrier.arrive_and_wait();
    std::cout << "... Keyboard Handler is starting.\n";

    running.store(true, std::memory_order_relaxed);

    CrossPlatformChar reader;      // provided by ScannerLibrary.cpp
    std::string buf;               // local input buffer
    buf.reserve(256);

    std::cout << "Type 'help' to see commands.\n> " << std::flush;

    // Poll characters in small slices to remain responsive to stop requests
    while (running.load(std::memory_order_relaxed)) {
      std::optional<char> ch = reader.getChar(slice);

      if (!ch.has_value()) {
        // no key within slice — loop again
        continue;
      }

      unsigned char c = static_cast<unsigned char>(*ch);

      // Handle Enter (CR or LF)
      if (c == '\n' || c == '\r') {
        std::cout << std::endl;
        if (!buf.empty() && commandHandlerCallback) {
          commandHandlerCallback(buf);
        }
        buf.clear();
        std::cout << "> " << std::flush;
        continue;
      }

      // Handle Backspace/Delete
      if (c == 127 || c == 8) { // 127=DEL (POSIX), 8=Backspace (Win)
        if (!buf.empty()) {
          buf.pop_back();
          std::cout << "\b \b" << std::flush; // visually erase last char
        }
        continue;
      }

      // Treat EOF shortcuts as "exit" (Ctrl+D on POSIX, Ctrl+Z on Windows)
#if defined(_WIN32)
      if (c == 26) { // Ctrl+Z
        if (commandHandlerCallback) commandHandlerCallback("exit");
        break;
      }
#else
      if (c == 4) {  // Ctrl+D
        if (commandHandlerCallback) commandHandlerCallback("exit");
        break;
      }
#endif

      // Ignore other control characters
      if (c < 32) continue;

      // Normal printable character
      buf.push_back(static_cast<char>(c));
      std::cout << static_cast<char>(c) << std::flush;
    }

    std::cout << "... Keyboard Handler stopped.\n";
  }

  // CommandHandler registers its "enqueue" callback here
  void connectHandler(std::function<void(const std::string&)> callbackF) {
    std::cout << "... Connecting Handler\n";
    commandHandlerCallback = std::move(callbackF);
  }

  // Called by MarqueeConsole when shutting down
  void requestStop() {
    running.store(false, std::memory_order_relaxed);
  }

private:
  const std::chrono::milliseconds slice{50}; // poll every 50ms
  std::function<void(const std::string&)> commandHandlerCallback;
  std::atomic<bool> running{false};
};
