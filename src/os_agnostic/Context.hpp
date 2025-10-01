/**
 * Shares the Marquee console's state across all handler threads.
 * 
 * This header follows the same comment style used in the original Context.cpp.
 */

#pragma once

#include <atomic>
#include <barrier>
#include <latch>
#include <mutex>
#include <string>
#include <thread>
#include <condition_variable>
#include <functional>
#include <iostream>

// >>> GLOBAL PARTICIPANT COUNT
#define NUM_MARQUEE_HANDLERS 4

// >>> BARRIER COMPLETION (kept noexcept for MSVC compatibility)
struct PhaseCompletion { void operator()() noexcept {} };

// >>> SHARED CONTEXT
struct MarqueeContext {

  public:

    // >>> PHASE & SHUTDOWN SYNC
    std::barrier<PhaseCompletion> phase_barrier{NUM_MARQUEE_HANDLERS}; // Waits for all threads to sync.
    std::latch                     stop_latch{NUM_MARQUEE_HANDLERS};   // Waits for all threads to finish.

    // >>> RUN/PAUSE STATE (names preserved as requested)
    bool pauseHandler() { std::lock_guard<std::mutex> lock{mtx}; pause = false; return pause; } // false = not paused (running)
    bool runHandler()   { std::lock_guard<std::mutex> lock{mtx}; pause = true;  return pause; } // true  = paused (per original naming)
    bool isRunning() const { return pause; } // Note: preserved semantics from original

    // >>> CONSOLE OUTPUT GUARD
    std::mutex coutMutex; // Prevent interleaved console output.

    // >>> GLOBAL EXIT FLAG
    std::atomic<bool> exitRequested{false};

    // >>> PROMPT & VIDEO FLAGS
    void setHasPromptLine(bool v) { hasPromptLine.store(v); }
    bool getHasPromptLine() const { return hasPromptLine.load(); }

    void setMarqueeActive(bool v) { marqueeActive.store(v); }
    bool isMarqueeActive() const { return marqueeActive.load(); }

    // >>> MARQUEE STATE
    std::mutex   textMutex;
    std::string  marqueeText{"Welcome to Marquee Console!"};
    std::atomic<int> speedMs{200};
    std::atomic<int> frameWidth{40};   // visible chars; default 40
    std::atomic<int> frameHeight{1};   // visible lines; default 1

    void setFrameSize(int w, int h) {
      if (w > 0) frameWidth.store(w);
      if (h > 0) frameHeight.store(h);
    }
    int getFrameWidth()  const { return frameWidth.load(); }
    int getFrameHeight() const { return frameHeight.load(); }


    void setText(const std::string &s) { std::lock_guard<std::mutex> lock(textMutex); marqueeText = s; }
    std::string getText() { std::lock_guard<std::mutex> lock(textMutex); return marqueeText; }

  private:
    std::atomic<bool> pause{false};
    std::atomic<bool> hasPromptLine{false};
    std::atomic<bool> marqueeActive{false};
    std::mutex mtx;
};

// >>> SHARED HANDLER CONSTRUCTOR SIG
class Handler {
  public:
    explicit Handler(MarqueeContext &c) : ctx(c) {}
    virtual ~Handler() = default;
  protected:
    MarqueeContext &ctx;
};
