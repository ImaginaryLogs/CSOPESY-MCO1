/**
 * @file KeyboardHandler.cpp
 * @brief Keyboard handler using OS-dependent Scanner for per-key responsiveness.
 */

#include "KeyboardHandler.hpp"
#include <iostream>

/**
* @brief Makes sure the cursor anchor and prompt line are displayed on the console.
*
* If the prompt hasn't been printed yet, (e.g., at startip), this results in
* The interactive prompt appears after two blank lines (for the status and marquee).
*
* @param ctx Shared context with display state and prompt.
*/
static void ensurePromptAnchor(MarqueeContext& ctx) {
    if (!ctx.getHasPromptLine()) {
        std::lock_guard<std::mutex> lock(ctx.coutMutex);

        std::cout << "\n\n"     // allocate [status] + [marquee] lines
                  << "> "       // print prompt
                  << "\x1b[s"   // save anchor at end of prompt
                  << std::flush;

        ctx.setHasPromptLine(true);
    }
}

/**
 * @brief Redraws the buffer text at the anchor position and the current prompt line.
 *
 * Always clears the line, moves the cursor to the prompt anchor (line beneath the marquee),
 * resaves the anchor and prints the prompt character and buffer.
 *
 * @param ctx Shared context with prompt state and cursor lock.
 * @param buf The input buffer that the user is currently typing.
*/
static void redrawPrompt(MarqueeContext& ctx, const std::string& buf) {
    std::lock_guard<std::mutex> lock(ctx.coutMutex);

    std::cout << "\x1b[u"         // restore to prompt anchor
              << "\r\x1b[2K> "    // clear prompt line and print prompt symbol
              << buf              // show buffer
              << "\x1b[s"         // re-save anchor at end-of-line
              << std::flush;

    ctx.setHasPromptLine(true);
}

/**
 * @brief The keyboard handler's main loop.
 *
 * Uses the platform-specific Scanner to wait for key input.
 * Delivers input on Enter after buffering it into a line, and manages
 * Manually press the special keys (Ctrl+C, Backspace).
*/

void KeyboardHandler::operator()() {
    // >>> JOIN INIT PHASE
    ctx.phase_barrier.arrive_and_wait();

    Scanner scan;
    std::string buffer;

    // Verify that the cursor anchor and prompt are prepared.
    ensurePromptAnchor(ctx);

    while (!ctx.exitRequested.load()) {
        // If something else cleared the prompt, re-anchor
        if (!ctx.getHasPromptLine()) {
            ensurePromptAnchor(ctx);
        }

        int ch = scan.poll();  // non-blocking input
        if (ch < 0) continue;  // no input yet

        if (ch == '\n') {
            // 1) The buffer disappears when the prompt line is cleared.
            const std::string submitted = buffer;
            buffer.clear();
            // redrawPrompt(ctx, buffer); // optional prompt clear (commented out for now)

            // 2) Deliver the command to the person who has signed up to receive it.
            if (deliver) deliver(submitted);

        } else if (ch == 3) {  // Ctrl+C pressed
            ctx.exitRequested.store(true);
            break;

        } else if (ch == 127 || ch == 8) {  // Backspace
            if (!buffer.empty()) {
                buffer.pop_back();
                redrawPrompt(ctx, buffer);
            }

        } else if (ch >= 32 && ch < 127) {  // Printable ASCII
            buffer.push_back(static_cast<char>(ch));
            redrawPrompt(ctx, buffer);
        }
    }

    // Clear prompt line on exit
    {
        std::lock_guard<std::mutex> lock(ctx.coutMutex);
        std::cout << "\x1b[u"     // return to prompt anchor
                  << "\r\x1b[2K"  // clear that line
                  << std::flush;
    }

    ctx.setHasPromptLine(false);

    // >>> THREAD EXIT
    ctx.stop_latch.count_down();
}
