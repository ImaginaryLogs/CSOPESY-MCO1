#include <mutex>
#include <condition_variable>
#pragma once

struct SyncContext
{
  std::mutex screenLock;
  std::condition_variable cv;
  // Indicate whose turn it is: Display or Input
  enum class Turn
  {
    Display,
    Input
  };
  Turn turn = Turn::Display;
  bool running = true;
};
