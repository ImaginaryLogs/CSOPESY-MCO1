#include <mutex>
#include <condition_variable>
#include <thread>
#include <latch>
#include <functional>
#include <iostream>
#include <barrier>

#pragma once

#define NUM_MARQUEE_HANDLERS 3

struct MarqueeContext
{
  public:
  std::barrier<std::function<void()>> phase_barrier{NUM_MARQUEE_HANDLERS, [this](){ std::cout << "\nAll handlers ready, starting up...\n" << std::endl; }};
  std::latch handlers_done{NUM_MARQUEE_HANDLERS};
  std::latch start_clean_up{1};
};

class Handler {
  public:
  MarqueeContext &ctx;

  explicit Handler(MarqueeContext &c) : ctx(c) {
    
  };

  Handler();

  ~Handler(){
    ctx.handlers_done.count_down();
  };

  virtual void operator()() = 0;
  
  bool pauseHandler()
  {
    std::lock_guard<std::mutex> lock{mtx};
    pause = false;
    return pause;
  }

  bool runHandler()
  {
    std::lock_guard<std::mutex> lock{mtx};
    pause = true;
    return pause;
  }

  bool isRunning() const {
    return pause;
  }

  private:
  std::atomic<bool> pause{false};
  std::mutex mtx;
  
};

