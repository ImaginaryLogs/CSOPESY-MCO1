/**
 * @file DisplayHandler.cpp
 * @brief renders the console's marquee frames.
 */

#include "DisplayHandler.hpp"
#include <chrono>
#include <thread>
#include <iostream>

#if defined(_WIN32)
#include <windows.h>

/**
 * @brief Allows Windows to support virtual terminals (for ANSI codes).
 */
static void enableVirtualTerminal() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;

    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) return;

    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
}

#else
// No-op on non-Windows platforms
static void enableVirtualTerminal() {}
#endif

/**
 * @brief wraps the first character to the end and scrolls the marquee text by one character.
 * @parameter s The marquee string that is currently in use.
 * After scrolling, @return the updated string.
 */
std::string DisplayHandler::scrollOnce(const std::string& s) {
    if (s.empty()) return s;
    std::string out = s.substr(1) + s.substr(0, 1);
    return out;
}

/**
 * @brief Main display loop that adds the marquee to the console.
 *
 * Awaits the phase barrier to be crossed by all handlers.
 * After that, it keeps looping, updating the marquee above the console prompt.
 * or, if the prompt hasn't been drawn yet, inline.
 */
void DisplayHandler::operator()() {
    // >>> JOIN INIT PHASE
    ctx.phase_barrier.arrive_and_wait();

    enableVirtualTerminal();

    std::string cur = ctx.getText();  // local copy of marquee string

    while (!ctx.exitRequested.load()) {
        if (ctx.isMarqueeActive()) {
            {
                std::lock_guard<std::mutex> guard(ctx.textMutex);
                cur = ctx.marqueeText;

                if (!cur.empty()) {
                    ctx.marqueeText = scrollOnce(cur);  // update shared state
                    cur = ctx.marqueeText;              // reflect locally
                }
            }

            {
                std::lock_guard<std::mutex> lock(ctx.coutMutex);
                if (ctx.getHasPromptLine()) {
                    // Draw the prompt line above and then move the cursor back to its original position.
                    std::cout << "\x1b[u"      // restore to prompt anchor
                              << "\x1b[1F"     // move one line up
                              << "\r\x1b[2K"   // clear that line
                              << cur
                              << "\x1b[u"      // restore to prompt again
                              << std::flush;
                } else {
                    // If no prompt yet, draw directly where we are
                    std::cout << "\r\x1b[2K" << cur << std::flush;
                }
            }
        }

        // Short sleep to regulate refresh rate according to the speed setting of the context
        std::this_thread::sleep_for(std::chrono::milliseconds(ctx.speedMs.load()));
    }

    // >>> THREAD EXIT
    ctx.stop_latch.count_down();  // signal this handler is finished
}
