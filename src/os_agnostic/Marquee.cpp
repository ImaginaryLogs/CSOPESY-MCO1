#include "DisplayHandler.cpp"
#include "KeyboardHandler.cpp"
#include "CommandHandler.cpp"
#include "Context.cpp"

#include <atomic>
#include <thread>
#include <vector>
#include <iostream>

class MarqueeConsole
{
public:
  MarqueeConsole()
  : videoName("CSU Marquee"),
    display(ctx),
    scanner(ctx),
    commandProcessor(ctx),
    isRunning(false)
  {}

  ~MarqueeConsole() { stop(); }

  // Entry point used by main.cpp
  void run()
  {
    if (isRunning.exchange(true)) {
      std::cout << "MarqueeConsole is already running.\n";
      return;
    }

    std::cout << "Running..." << std::endl;

    // 1) Wire: Command -> Display, Keyboard -> Command
    commandProcessor.connectDisplayHandler(display);
    scanner.connectHandler(commandProcessor.getCommandCallback());

    // 2) Start workers AFTER wiring so no keystroke is lost
    worker_threads.emplace_back(std::ref(display));
    worker_threads.emplace_back(std::ref(commandProcessor));
    worker_threads.emplace_back(std::ref(scanner));

    // 3) Block until the command thread decides to exit
    //    (typing `exit` will request shutdown)
    worker_threads[1].join(); // commandProcessor finishes when 'exit' is issued

    // 4) Ask the keyboard to stop polling and join others
    scanner.requestStop();

    // 5) Join remaining threads
    if (worker_threads[2].joinable()) worker_threads[2].join(); // scanner
    if (worker_threads[0].joinable()) worker_threads[0].join(); // display

    isRunning = false;
    std::cout << "Console stopped.\n";
  }

  void stop()
  {
    if (!isRunning.exchange(false)) return;

    std::cout << "Exiting..." << std::endl;

    // Best-effort: stop keyboard polling so joins don’t hang
    scanner.requestStop();

    for (auto &t : worker_threads) {
      if (t.joinable()) t.join();
    }
    worker_threads.clear();
  }

private:
  std::string videoName;

  MarqueeContext   ctx;
  DisplayHandler   display;
  KeyboardHandler  scanner;
  CommandHandler   commandProcessor;

  std::atomic<bool> isRunning;
  std::vector<std::thread> worker_threads;
};
