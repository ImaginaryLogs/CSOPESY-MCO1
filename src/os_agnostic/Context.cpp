#include <mutex>
#include <condition_variable>
#include <thread>
#include <latch>
#include <functional>
#include <iostream>

#pragma once

#define MARQUEE_HANDLERS 4


struct MarqueeContext
{
  std::mutex screenLock;
  std::condition_variable cv;
  
  // Indicate whose turn it is: Display or Input
  enum class Turn {
    Display,
    Input
  };
  Turn turn = Turn::Display;
  
  std::atomic<bool> running = true;
  std::latch handlers_done{MARQUEE_HANDLERS};
  std::latch start_clean_up{1};
};

class Handler {
  Handler
  
  virtual void operator()() = 0;


}