#include <string>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include "contract.cpp"
#include <functional>

class ScreenDisplay
{
public:
  explicit ScreenDisplay(SyncContext &ctx) : ctx(ctx) {}

  void operator()()
  {
    using clock = std::chrono::steady_clock;
    const auto slice = std::chrono::milliseconds(500);

    while (true)
    {
      std::unique_lock<std::mutex> lock(ctx.screenLock);
      ctx.cv.wait(lock, displayTurnPredicate);
      
      if (!ctx.running) break;
      
      auto deadline = clock::now() + slice;

      clearScreen();
      while (clock::now() < deadline && ctx.running){
        lock.unlock();
        std::cout << getCurrentFrame() << std::flush;

        lock.lock();

        if (!ctx.running) break;
      }

      ctx.turn = SyncContext::Turn::Input;
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
  void clearScreen(){
    std::cout << "\033[2J\033[1;1H";
  };

private:
  SyncContext &ctx;
  bool running = false;
  std::string getCurrentFrame(){
    return "Video Frame Test\n";
  };

  bool isDisplayTurn(){ return ctx.turn == SyncContext::Turn::Display || !ctx.running;};

  // Lambda Expression for condition variable
  std::function<bool()> displayTurnPredicate = [this]()
  { return isDisplayTurn(); };
};
