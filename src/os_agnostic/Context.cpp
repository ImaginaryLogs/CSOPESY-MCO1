/**
 * Shares the Marquee console's state across all handler threads.
 */

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

// MSVC 2019 apparently requires this workaround
// https://developercommunity.visualstudio.com/t/c20-barrier-not-compiling/101958
struct PhaseCompletion { void operator()() noexcept {} };

// >>> SHARED MARQUEE STATE
struct MarqueeContext {

public:

  // >>> THREADS

    // Thread synchronizations.
    std::barrier<PhaseCompletion> phase_barrier{NUM_MARQUEE_HANDLERS}; // Waits for all threads to sync.
    std::latch stop_latch{NUM_MARQUEE_HANDLERS};                       // Waits for all threads to finish.

    // Handler states.
    bool pauseHandler() { std::lock_guard<std::mutex> lock{mtx}; pause = false; return pause; }
    bool runHandler()   { std::lock_guard<std::mutex> lock{mtx}; pause = true;  return pause; }
    bool isRunning() const { return pause; }

  // >>> MUTEXES

    std::mutex coutMutex; // Console output mutex.
    std::mutex textMutex; // Marquee text access mutex.

  // >>> FLAGS

    // Exit flag (for all threads).
    std::atomic<bool> exitRequested{false}; 

    // Prompt presence flag.
    void setHasPromptLine(bool v) { hasPromptLine.store(v); }
    bool getHasPromptLine() const { return hasPromptLine.load(); }

    // Marquee status flag.
    void setMarqueeActive(bool v) { marqueeActive.store(v); }
    bool isMarqueeActive() const { return marqueeActive.load(); }

  // >>> MARQUEE

    // Marquee content and speed.
    std::string marqueeText{"Welcome to Marquee Console!"};
    std::atomic<int> speedMs{200};

    // Marquee text accessors with locking.
    void setText(const std::string &s) { std::lock_guard<std::mutex> lock(textMutex); marqueeText = s; }
    std::string getText() { std::lock_guard<std::mutex> lock(textMutex); return marqueeText; }

// >>> INTERNAL STATE
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