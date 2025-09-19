/*
    This is the DISPLAY worker thread. It does three things:
    1) waits on the startup barrier (so all threads begin together)
    2) asks AnimationHandler for a frame (centered or scrolling)
    3) prints that frame and sleeps for speedMs

    Note: I use ANSI "in-place redraw" so the box doesn't flicker by clearing the
    whole console. On Windows 10+, enable VT in main() (ENABLE_VIRTUAL_TERMINAL_PROCESSING).
*/

#include <string>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <chrono>
#include <vector>

#include "Context.cpp"
#include "AnimationHandler.cpp"  // project style: include .cpp directly

class DisplayHandler : public Handler {
public:
    explicit DisplayHandler(MarqueeContext& c) : Handler(c) {}

    // thread entrypoint
    void operator()() {
        std::cout << "... Display Handler is waiting.\n";
        ctx.phase_barrier.arrive_and_wait();
        std::cout << "... Display Handler is starting.\n";

        // show an initial centered frame (so it's not blank before start)
        drawFrame(/*scrolling*/false);

        while (!terminate_.load(std::memory_order_relaxed)) {
            // wait until either "running" is true or termination is requested
            {
                std::unique_lock<std::mutex> lk(mtx_);
                cv_.wait_for(lk, std::chrono::milliseconds(10), [&] {
                    return terminate_.load(std::memory_order_relaxed) ||
                        running_.load(std::memory_order_relaxed);
                    });
                if (terminate_.load(std::memory_order_relaxed)) break;
            }

            if (!running_.load(std::memory_order_relaxed)) {
                // paused: keep the centered frame on screen
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                continue;
            }

            drawFrame(/*scrolling*/true);
            std::this_thread::sleep_for(std::chrono::milliseconds(anim_.speedMs()));
        }

        std::cout << "\n... Display Handler stopped.\n";
    }

    // ---- Control API (these are what CommandHandler calls) ----

    void start() {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            running_.store(true, std::memory_order_relaxed);
        }
        cv_.notify_all();
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            running_.store(false, std::memory_order_relaxed);
            anim_.resetScroll();
        }
        drawFrame(/*scrolling*/false);
    }

    void setText(const std::string& s) {
        anim_.setText(s);
        if (!running_.load(std::memory_order_relaxed)) {
            drawFrame(/*scrolling*/false);
        }
    }

    void setSpeed(int ms) {
        anim_.setSpeed(ms);
    }

    // optional extension (not required by some PDFs, but handy)
    void setSize(int w, int h) {
        anim_.setSize(w, h);
        drawFrame(running_.load(std::memory_order_relaxed));
    }

    bool isRunning() const { return running_.load(std::memory_order_relaxed); }

    // simple diagnostics (used by "video ping" in some skeletons)
    void ping() { std::cout << "Display Handler received ping...\n"; }
    void displayString(const std::string& s) { std::cout << "Video Thread: " << s << std::endl; }

    void requestExit() {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            terminate_.store(true, std::memory_order_relaxed);
            running_.store(false, std::memory_order_relaxed);
        }
        cv_.notify_all();
    }

private:
    void drawFrame(bool scrolling) {
        const std::vector<std::string> lines = anim_.buildFrame(scrolling);
        const int H = static_cast<int>(lines.size());

        if (hasDrawnOnce_) {
            // move cursor UP by H lines so we overwrite in place
            std::cout << "\x1b[" << H << "A";
        }
        else {
            hasDrawnOnce_ = true;
        }

        for (int i = 0; i < H; ++i) {
            std::cout << lines[i] << (i + 1 == H ? "" : "\n");
        }
        std::cout << std::flush;
    }

private:
    // basic flags/coordination
    std::mutex mtx_;
    std::condition_variable cv_;
    std::atomic<bool> running_{ false };
    std::atomic<bool> terminate_{ false };
    bool hasDrawnOnce_{ false };

    // the logic core
    AnimationHandler anim_;
};
