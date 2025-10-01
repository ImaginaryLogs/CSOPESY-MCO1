/**
 * @file KeyboardHandler.hpp
 * @brief reads input from the user and sends it as commands.
 */

#pragma once

#include "Context.hpp"
#include "../os_dependent/Scanner.hpp"
#include <string>
#include <functional>

/**
 * @brief pushes commands into the system and manages keyboard input.
 *
 * Looks for important events using a platform-specific scanner. When a line
 * is finished (by pressing Enter), it is sent to a command processor through the
 * setSink, the configured callback.
*/

class KeyboardHandler : public Handler {
public:
    /**
     * @brief Provide access to shared context when building the handler.
     * @param c All components use the same MarqueeContext.
     */
    explicit KeyboardHandler(MarqueeContext& c) : Handler(c) {}

    /**
     * @brief Keyboard input is read and interpreted by the main loop.
     *
     * Responds to user input and manages per-keystroke actions such as
     * Newline, character typing, and backspace. Moreover, it leaves cleanly.
     * on Ctrl+C.
     */
    void operator()();

    /**
     * @brief Configures the function to execute upon entering a complete command.
     * @param sink A function that takes a line of completed input.
     */
    void setSink(std::function<void(std::string)> sink) {
        deliver = std::move(sink);  // injection point to deliver commands to command processor
    }

private:
    std::function<void(std::string)> deliver;  // holds the command sink callback
};
