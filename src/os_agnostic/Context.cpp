#include <barrier>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <latch>
#include <mutex>
#include <thread>
#include <atomic>
#include <string>

#pragma once

#define NUM_MARQUEE_HANDLERS 4

// MSVC 2019 requires a noexcept-invocable barrier completion.
struct PhaseCompletion { void operator()() noexcept {} };

// Shared state and synchronization primitives for the marquee console.
struct MarqueeContext {
public:
  std::barrier<PhaseCompletion> phase_barrier{NUM_MARQUEE_HANDLERS};
  std::latch stop_latch{NUM_MARQUEE_HANDLERS};

  // Basic run/pause state (names preserved to follow given skeleton).
  bool pauseHandler() { std::lock_guard<std::mutex> lock{mtx}; pause = false; return pause; }
  bool runHandler()   { std::lock_guard<std::mutex> lock{mtx}; pause = true;  return pause; }
  bool isRunning() const { return pause; }

  // Console output mutex to prevent interleaved lines.
  std::mutex coutMutex;

  // Global exit flag.
  std::atomic<bool> exitRequested{false};

  // Prompt presence flag (renderer anchors one line above it).
  void setHasPromptLine(bool v) { hasPromptLine.store(v); }
  bool getHasPromptLine() const { return hasPromptLine.load(); }

  // NEW: whether marquee is actively rendering frames (keyboard uses this to decide newline)
  void setMarqueeActive(bool v) { marqueeActive.store(v); }
  bool isMarqueeActive() const { return marqueeActive.load(); }

  // Marquee state (shared)
  std::mutex textMutex;
  std::string marqueeText{"Welcome to Marquee Console!"};
  std::atomic<int> speedMs{200};

  void setText(const std::string &s) { std::lock_guard<std::mutex> lock(textMutex); marqueeText = s; }
  std::string getText() { std::lock_guard<std::mutex> lock(textMutex); return marqueeText; }

private:
  std::atomic<bool> pause{false};
  std::atomic<bool> hasPromptLine{false};
  std::atomic<bool> marqueeActive{false};   // <-- insertion
  std::mutex mtx;
};

// Lightweight base so all handlers share the same constructor signature.
class Handler {
public:
  explicit Handler(MarqueeContext &c) : ctx(c) {}
  virtual ~Handler() = default;
protected:
  MarqueeContext &ctx;
};
