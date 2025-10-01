/**
 * @file MarqueeConsole.hpp
 * @brief Mainly handles the starting and shutting down of all threads.
 */

#pragma once

#include "Context.hpp"
#include "DisplayHandler.hpp"
#include "KeyboardHandler.hpp"
#include "CommandHandler.hpp"
#include <thread>
#include <vector>

/**
 * @brief The top-level console controller that connects everything.
 *
 * Oversees the marquee system's setup and shutdown, including the display,
 * keyboard input and command parsing.
 * Aside from initiation, threads wire together here.
*/
class MarqueeConsole {
public:
    
    // Constructs the console and initializes connections of the handlers.
    MarqueeConsole();

    /**
     * @brief starts the console system and keeps it running until it shuts down.
     *
     * Launches every worker thread, including the supervisor thread that
     * watches for the exit signal. Joins everything at shutdown.
     */
    void run();

private:
    MarqueeContext ctx;                     // shared state across all handlers
    DisplayHandler display;                 // renders the animated marquee onto the console
    KeyboardHandler keyboard;               // captures inputs from keystrokes
    CommandHandler command;                 // processes and executes the corresponding actions of commands
    std::vector<std::thread> threads;       // all handler and supervisor threads
};
