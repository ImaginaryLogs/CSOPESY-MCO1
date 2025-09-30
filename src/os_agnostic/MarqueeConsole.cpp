/**
 * High-level console wiring: starts threads and coordinates shutdown.
 */

#include "MarqueeConsole.hpp"
#include <iostream>
#include <chrono>

MarqueeConsole::MarqueeConsole()
: ctx()
, display(ctx)
, keyboard(ctx)
, command(ctx)
, fileReader(ctx)
{
  command.addDisplayHandler(&display);
  command.addFileReaderHandler(&fileReader);
  keyboard.setSink([this](std::string cmd){ command.enqueue(std::move(cmd)); });
}

void MarqueeConsole::run() {
  // launch handlers
  threads.emplace_back(std::ref(display));
  threads.emplace_back(std::ref(keyboard));
  threads.emplace_back(std::ref(command));

  // A fourth participant for barrier & latch: the "supervisor" thread lambda
  threads.emplace_back([this]{
    // >>> JOIN INIT PHASE
    ctx.phase_barrier.arrive_and_wait();
    // supervisor waits for exit
    while (!ctx.exitRequested.load()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    {
      std::lock_guard<std::mutex> lock(ctx.coutMutex);
      std::cout << "\nExiting..." << std::endl;
    }
    ctx.stop_latch.count_down();
  });

  // Wait for all threads to finish
  ctx.stop_latch.wait();
  for (auto& t : threads) if (t.joinable()) t.join();
}
