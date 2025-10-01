/**
 * @file CommandHandler.hpp
 * @brief Simple command runner for the marquee console.
 *
 * This class reads command strings from a queue (usually pushed by the keyboard
 * thread) and runs them. It talks to the display handler (for starting/stopping
 * the marquee) and the file reader (for loading text).
 *
 * Commands from the spec:
 *   - help
 *   - exit
 *   - start_marquee
 *   - stop_marquee
 *   - set_speed <ms>
 *   - set_text <text>
 *
 * Extra (optional):
 *   - load_file <path>
 */

#pragma once

#include "Context.hpp"
#include "DisplayHandler.hpp"
#include "FileReaderHandler.hpp"

#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

/**
 * @class CommandHandler
 * @brief Consumes command lines and executes them, one by one.
 *
 * How to use:
 *  - Build it with a shared MarqueeContext.
 *  - Hook up the DisplayHandler and FileReaderHandler (if you have them).
 *  - Run operator() on its own thread so it can block/wake on the queue.
 *  - Call enqueue() from any producer (like the keyboard thread).
 */
class CommandHandler : public Handler {
public:
    /**
     * @brief Make a handler that uses the given shared context.
     * @param c Shared MarqueeContext with all the shared flags, locks, etc.
     */
    explicit CommandHandler(MarqueeContext& c) 
        : Handler(c), display(nullptr), fileReader(nullptr) {
        // Note: Display and fileReader are optional and can be set later
    }

    /**
     * @brief Main loop that waits for commands and executes them.
     *
     * Blocks on a condition variable when the queue is empty. Exits when
     * ctx.exitRequested becomes true.
     */
    void operator()(); // consumer loop

    /**
     * @brief Provide a pointer to the display handler so we can start/stop the marquee.
     * @param d DisplayHandler to use (can be nullptr to detach).
     */
    void addDisplayHandler(DisplayHandler* d) {
        display = d;  // nothing fancy here
    }

    /**
     * @brief Provide a pointer to the file reader so we can load text from a file.
     * @param f FileReaderHandler to use (can be nullptr to detach).
     */
    void addFileReaderHandler(FileReaderHandler* f) {
        fileReader = f;
    }

    /**
     * @brief Push a new command line into the queue.
     *
     * Thread-safe. Multiple producers can call this at the same time.
     *
     * @param cmd Raw command, e.g. "set_speed 120".
     */
    void enqueue(std::string cmd);

private:

    // >>> QUEUE STATE

    std::mutex queueMutex;                  // Protects access to the queue.
    std::condition_variable queueCv;        // Signals the consumer that there is work to do (or we are exiting)
    std::queue<std::string> commandQueue;   // Ensure command strings follow FIFO

    // >>> POINTERS TO BE CALLED

    DisplayHandler* display;          // Display handler used to start/stop marquee
    FileReaderHandler* fileReader;    // File reader (extra feature)

    // >>> HELPERS
    
    /**
     * @brief Parse one command line and do the action.
     * @param line Full command line including any arguments.
     */
    void handleCommand(const std::string& line);  // Note: might split this up later

    /**
     * @brief Print the help text with the supported commands.
     */
    void printHelp();
};
