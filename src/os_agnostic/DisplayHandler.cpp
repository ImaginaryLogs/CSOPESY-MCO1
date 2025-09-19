#include <string>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <chrono>

#include "Context.cpp"

// Renders a simple scrolling marquee to stdout.
class DisplayHandler : public Handler {
public:
  explicit DisplayHandler(MarqueeContext& c) : Handler(c) {}

  void operator()() {
    std::cout << "... Display Handler is waiting.\n";
    ctx.phase_barrier.arrive_and_wait();
    std::cout << "... Display Handler is starting.\n";

    while (!terminate.load(std::memory_order_relaxed)) {
      // Wait until started or told to exit
      {
        std::unique_lock<std::mutex> lk(mtx);
        cv.wait(lk, [&]{
          return terminate.load(std::memory_order_relaxed) ||
                 running.load(std::memory_order_relaxed);
        });
        if (terminate.load(std::memory_order_relaxed)) break;
      }

      // snapshot state under lock
      std::string textCopy;
      int speedMsCopy;
      {
        std::lock_guard<std::mutex> lk(mtx);
        textCopy = text;
        speedMsCopy = speedMs;
      }

      if (textCopy.empty()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(60));
        continue;
      }

      // rotate one step and print on same line
      std::string frame = nextFrame(textCopy);
      std::cout << "\r" << frame << std::flush;

      std::this_thread::sleep_for(std::chrono::milliseconds(speedMsCopy));
    }

    std::cout << "\n... Display Handler stopped.\n";
  }

  // small utilities used by CommandHandler
  void start() { 
    { std::lock_guard<std::mutex> lk(mtx); running.store(true); } 
    cv.notify_all(); 
  }
  void stop()  { std::lock_guard<std::mutex> lk(mtx); running.store(false); }
  void setText(const std::string& s) { std::lock_guard<std::mutex> lk(mtx); text = s; offset = 0; }
  void setSpeed(int ms) { std::lock_guard<std::mutex> lk(mtx); speedMs = std::max(1, ms); }
  bool isRunning() const { return running.load(); }

  void ping() { std::cout << "Display Handler received ping...\n"; }
  void displayString(const std::string &s) { std::cout << "Video Thread: " << s << std::endl; }

  void requestExit() {
    { std::lock_guard<std::mutex> lk(mtx); terminate.store(true); running.store(false); }
    cv.notify_all();
  }

private:
  std::string nextFrame(const std::string& src) {
    static const std::string pad = "   ";
    const std::string loop = src + pad;
    if (offset >= loop.size()) offset = 0;
    std::string out = loop.substr(offset) + loop.substr(0, offset);
    ++offset;
    return out;
  }

  // state
  std::mutex mtx;
  std::condition_variable cv;
  std::atomic<bool> running{false};
  std::atomic<bool> terminate{false};

  std::string text{"CSU Marquee Emulator"};
  int speedMs{120};
  size_t offset{0};
};
