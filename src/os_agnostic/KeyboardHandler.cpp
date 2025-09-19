/*
    Reads characters (with a tiny poll delay), handles Enter/Backspace, and
    forwards complete lines to the CommandHandler via a callback.
*/


#include "Context.cpp"
#include "../os_dependent/ScannerLibrary.cpp"
#include <iostream>
#include <string>
#include <chrono>
#include <functional>
#include <atomic>
#include <optional>

class KeyboardHandler : public Handler {
public:
    explicit KeyboardHandler(MarqueeContext& c) : Handler(c) {}

    void operator()() {
        std::cout << "... Keyboard Handler is waiting.\n";
        ctx.phase_barrier.arrive_and_wait();
        std::cout << "... Keyboard Handler is starting.\n";

        running_.store(true, std::memory_order_relaxed);
        CrossPlatformChar reader;
        std::string buf;
        buf.reserve(256);

        std::cout << "Type 'help' to see commands.\n> " << std::flush;

        while (running_.load(std::memory_order_relaxed)) {
            std::optional<char> ch = reader.getChar(slice_);
            if (!ch.has_value()) continue; // nothing yet

            unsigned char c = static_cast<unsigned char>(*ch);

            // Enter -> send line
            if (c == '\n' || c == '\r') {
                std::cout << std::endl;
                if (!buf.empty() && commandCb_) commandCb_(buf);
                buf.clear();
                std::cout << "> " << std::flush;
                continue;
            }

            // Backspace/Delete
            if (c == 127 || c == 8) {
                if (!buf.empty()) {
                    buf.pop_back();
                    std::cout << "\b \b" << std::flush;
                }
                continue;
            }

#if defined(_WIN32)
            if (c == 26) { // Ctrl+Z -> treat as "exit"
                if (commandCb_) commandCb_("exit");
                break;
            }
#else
            if (c == 4) {  // Ctrl+D
                if (commandCb_) commandCb_("exit");
                break;
            }
#endif

            if (c < 32) continue; // ignore other control chars

            // printable
            buf.push_back(static_cast<char>(c));
            std::cout << static_cast<char>(c) << std::flush;
        }

        std::cout << "... Keyboard Handler stopped.\n";
    }

    void connectHandler(std::function<void(const std::string&)> cb) {
        std::cout << "... Connecting Handler\n";
        commandCb_ = std::move(cb);
    }

    void requestStop() { running_.store(false, std::memory_order_relaxed); }

private:
    const std::chrono::milliseconds slice_{ 50 }; // poll every 50ms
    std::function<void(const std::string&)> commandCb_;
    std::atomic<bool> running_{ false };
};
