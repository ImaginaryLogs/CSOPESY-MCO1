// Marquee.cpp
#include "DisplayHandler.cpp"
#include "KeyboardHandler.cpp"
#include "CommandHandler.cpp"
#include "Context.cpp"
#include <thread>
#include <mutex>
#include <vector>

class MarqueeConsole
{
private:
  std::string videoName{"CSU Marquee"};
  MarqueeContext ctx;

  DisplayHandler   display{ctx};
  KeyboardHandler  scanner{ctx};
  CommandHandler   commandProcessor{ctx};

  std::vector<std::thread> worker_threads;

public:
  // Call this from main()
  void start()
  {
    // 1) Wire: Command drives Display, Keyboard feeds Command
    commandProcessor.connectDisplayHandler(display);
    scanner.connectHandler(commandProcessor.getCommandCallback());

    // 2) Spawn worker threads in any order AFTER wiring
    worker_threads.emplace_back(std::ref(display));
    worker_threads.emplace_back(std::ref(commandProcessor));
    worker_threads.emplace_back(std::ref(scanner));
  }

  void stop()
  {
    std::cout << "Exiting..." << std::endl;
    for (auto &t : worker_threads) {
      if (t.joinable()) t.join();
    }
  }
};

// (Optional) simple main if you don't already have one
int main() {
  MarqueeConsole app;
  app.start();
  app.stop();
  return 0;
}
