/*
    "App" wrapper that wires the three handlers together and manages threads.
    Wire first, THEN start threads, THEN join.
*/

#include "DisplayHandler.cpp"
#include "KeyboardHandler.cpp"
#include "CommandHandler.cpp"
#include "Context.cpp"

#include <atomic>
#include <thread>
#include <vector>
#include <iostream>

class MarqueeConsole {
public:
    MarqueeConsole()
        : display_(ctx_),
        keyboard_(ctx_),
        command_(ctx_) {
    }

    ~MarqueeConsole() { stop(); }

    void run() {
#ifdef _WIN32
        enableVT(); // makes ANSI redraw work nicely on Windows 10+ terminals
#endif

        if (isRunning_.exchange(true)) {
            std::cout << "MarqueeConsole is already running.\n";
            return;
        }

        std::cout << "Running..." << std::endl;

        // Wire: command -> display, keyboard -> command
        command_.connectDisplayHandler(display_);
        keyboard_.connectHandler(command_.getCommandCallback());

        // Start AFTER wiring so we don't miss early keystrokes
        threads_.emplace_back(std::ref(display_));
        threads_.emplace_back(std::ref(command_));
        threads_.emplace_back(std::ref(keyboard_));

        // Shut down cleanly when "exit" is issued
        threads_[1].join();                 // command finishes first
        keyboard_.requestStop();
        if (threads_[2].joinable()) threads_[2].join();
        if (threads_[0].joinable()) threads_[0].join();

        isRunning_ = false;
        std::cout << "Console stopped.\n";
    }

    void stop() {
        if (!isRunning_.exchange(false)) return;
        keyboard_.requestStop();
        for (auto& t : threads_) if (t.joinable()) t.join();
        threads_.clear();
    }

private:
#ifdef _WIN32
    // Small helper: enable ANSI/VT on Windows 10+ so in-place redraw works.
    void enableVT() {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut == INVALID_HANDLE_VALUE) return;
        DWORD mode = 0;
        if (!GetConsoleMode(hOut, &mode)) return;
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, mode);
    }
#endif

    MarqueeContext  ctx_;
    DisplayHandler  display_;
    KeyboardHandler keyboard_;
    CommandHandler  command_;

    std::atomic<bool> isRunning_{ false };
    std::vector<std::thread> threads_;
};
