/**
 * @file Context.hpp
 * @brief Shares the state of the Marquee console to every handler thread.
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
#define NUM_MARQUEE_HANDLERS 4  // Can be increased when more threads are added.

// >>> BARRIER COMPLETION (kept noexcept for MSVC compatibility)
struct PhaseCompletion {
    void operator()() noexcept {}
};

/**
 * @brief All marquee handlers share a thread-safe context.
 *
 * Holds console-related information, shared state flags, and synchronization primitives.
 * CommandHandler, DisplayHandler, and possibly other components use it.
 */
struct MarqueeContext {
public:

    // >>> PHASE & SHUTDOWN SYNC

    /** @brief Before beginning, all participating threads are synched with this barrier. */
    std::barrier<PhaseCompletion> phase_barrier{NUM_MARQUEE_HANDLERS};

    /** @brief Latch (threads call count_down()) to orchestrate smooth shutdown. */
    std::latch stop_latch{NUM_MARQUEE_HANDLERS};

    // >>> RUN/PAUSE STATE (names preserved as requested)

    /** @brief Resume the handler (pause turns to false). */
    bool pauseHandler() {
        std::lock_guard<std::mutex> lock{mtx};
        pause = false;
        return pause;
    }

    /** @brief Pause the handler (pause turns to true). */
    bool runHandler() {
        std::lock_guard<std::mutex> lock{mtx};
        pause = true;
        return pause;
    }

    /** @brief Verify whether the handler is running at the moment (note: true indicates paused). */
    bool isRunning() const {
        return pause;
    }

    // >>> CONSOLE OUTPUT GUARD
    
    std::mutex coutMutex; // Mutex to stop console writes in parallel.

    // >>> GLOBAL EXIT FLAG

    std::atomic<bool> exitRequested{false}; // Used to alert all threads to shutdown.

    // >>> PROMPT & VIDEO FLAGS

    /** @brief Configure the flag to know whether the prompt is visible. */
    void setHasPromptLine(bool v) {
        hasPromptLine.store(v);
    }

    /** @brief Verify whether the console prompt is visible at the moment. */
    bool getHasPromptLine() const {
        return hasPromptLine.load();
    }

    /** @brief Determine if the marquee is in an active display state. */
    void setMarqueeActive(bool v) {
        marqueeActive.store(v);
    }

    /** @brief Verify whether the marquee is active. */
    bool isMarqueeActive() const {
        return marqueeActive.load();
    }

    // >>> MARQUEE STATE
    
    std::mutex textMutex; // Lock guards the text buffer's access
    std::string marqueeText{"Welcome to Marquee Console!"}; // Current marquee text (which may be loaded from a file or set by the user). 
    std::atomic<int> speedMs{200}; // The marquee scroll's speed in milliseconds.

    /** @brief Change the text on the marquee. */
    void setText(const std::string &s) {
        std::lock_guard<std::mutex> lock(textMutex);
        marqueeText = s;
    }

    /** @brief Get a copy of the text that is currently displayed in the marquee. */
    std::string getText() {
        std::lock_guard<std::mutex> lock(textMutex);
        return marqueeText;
    }

private:
    std::atomic<bool> pause{false};
    std::atomic<bool> hasPromptLine{false};
    std::atomic<bool> marqueeActive{false};
    std::mutex mtx;
};

/**
 * @brief For any handler requiring access to the context's shared state.
 *
 * This is extended by all major handler types (e.g., DisplayHandler, CommandHandler).
 */
class Handler {
public:
    explicit Handler(MarqueeContext& c) : ctx(c) {}
    virtual ~Handler() = default;

protected:
    MarqueeContext& ctx;
};
