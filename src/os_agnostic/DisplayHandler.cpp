#include <string>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include "Context.cpp"
#include <functional>

class ScreenDisplay
{
public:
  explicit ScreenDisplay(MarqueeContext &ctx) : ctx(ctx) {}

  void operator()()
  {
    using clock = std::chrono::steady_clock;
    const auto slice = std::chrono::milliseconds(5);

    while (true)
    {
      std::unique_lock<std::mutex> lock(ctx.screenLock);
      ctx.cv.wait(lock, displayTurnPredicate);

      if (!ctx.running)
        break;

      auto deadline = clock::now() + slice;

      clearScreen();
      int turns = 0;
      while ((clock::now() < deadline && turns++ < 10) && ctx.running)
      {
        lock.unlock();
        std::chrono::duration<double> time = clock::now().time_since_epoch();
        std::cout << time.count() << getCurrentFrame() << std::flush;

        lock.lock();

        if (!ctx.running)
          break;
      }
      std::cout << "\n"
                << std::flush;

      ctx.turn = MarqueeContext::Turn::Input;
      lock.unlock();
      ctx.cv.notify_all();
    }
  };

  bool stopVideo()
  {
    running = false;
    return running;
  }
  void setVideo(const std::string &videoName);
  void clearScreen()
  {
    std::cout << "\033[2J\033[1;1H";
  };

private:
  MarqueeContext &ctx;
  bool running = false;
  std::string getCurrentFrame()
  {
    return "Video Frame Test\n";
  };

  bool isDisplayTurn() { return ctx.turn == MarqueeContext::Turn::Display || !ctx.running; };

  // Lambda Expression for condition variable
  std::function<bool()> displayTurnPredicate = [this]()
  { return isDisplayTurn(); };
};
