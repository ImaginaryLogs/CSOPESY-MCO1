/**
 * @file MarqueeConsole.cpp
 * @brief Mainly handles the starting and shutting down of all threads.
 */

#include "MarqueeConsole.hpp"
#include <iostream>
#include <chrono>

/**
 * @brief Wires up internal handlers and builds the console.
 *
 * Configures the command, keyboard, and display.
 * and uses callback binding and handler injection to connect them.
 */
MarqueeConsole::MarqueeConsole():
    ctx(),
    display(ctx),
    keyboard(ctx),
    command(ctx)
{
    // Hands off the display to the command processor.
    command.addDisplayHandler(&display);

    // Commands entered by the user are given to the command processor via the keyboard.
    keyboard.setSink([this](std::string cmd) {
        command.enqueue(std::move(cmd));
    });
}

/**
 * @brief manages the lifecycle until shutdown, launches all worker threads.
 *
 * Launches a supervisor thread along with the handlers for display, keyboard input, and commands.
 * This is idle until shutdown is requested, after waiting for all threads to reach the init barrier.
 */
void MarqueeConsole::run() {
    // Launch core handler threads
    threads.emplace_back(std::ref(display));
    threads.emplace_back(std::ref(keyboard));
    threads.emplace_back(std::ref(command));

    // A fourth participant in the barrier: the supervisor thread
    threads.emplace_back([this] {
        // >>> JOIN INIT PHASE
        ctx.phase_barrier.arrive_and_wait();

        // Supervisor loop: spin until exit is requested
        while (!ctx.exitRequested.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        // Optional user feedback (removed this bc of duplicates)
        // {
        //     std::lock_guard<std::mutex> lock(ctx.coutMutex);
        //     std::cout << "\nExiting..." << std::endl;
        // }

        // Count down to let others know we're done
        ctx.stop_latch.count_down();
    });

    // Wait until all threads finish gracefully
    ctx.stop_latch.wait();
    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }
}
