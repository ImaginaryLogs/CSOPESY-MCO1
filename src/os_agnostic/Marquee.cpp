#include "CommandHandler.cpp"
#include "Context.cpp"
#include "FileReaderHandler.cpp"
#include "KeyboardHandler.cpp"
#include <mutex>
#include <thread>
#include <vector>
#include <iostream>

class MarqueeConsole {
private:
  std::string videoName;
  DisplayHandler display;
  KeyboardHandler scanner;
  CommandHandler commandProcessor;
  FileReaderHandler fileReader;
  std::atomic<bool> isRunning;
  MarqueeContext ctx;

public:
  MarqueeConsole()
      : videoName("os_marquee"),
        display(ctx),
        scanner(ctx),
        commandProcessor(ctx),
        fileReader(ctx),
        isRunning(false),
        ctx() {}

  void run() {
    isRunning.store(true);

    // connect keyboard -> command handler
    scanner.connectHandler([this](const std::string &input) {
      commandProcessor.addInput(input);
    });
    commandProcessor.addDisplayHandler(&display);
    commandProcessor.addFileReaderHandler(&fileReader);

    // start threads (4)
    worker_threads.emplace_back(std::ref(display));
    worker_threads.emplace_back(std::ref(scanner));
    worker_threads.emplace_back(std::ref(commandProcessor));
    worker_threads.emplace_back(std::ref(fileReader));

    // Wait until all threads call barrier and then continue running until exit
    while (!ctx.exitRequested.load()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    stop();
  }

private:
  std::vector<std::thread> worker_threads;

  void stop() {

    {
      std::lock_guard<std::mutex> lock(ctx.coutMutex);
      std::cout << "Exiting..." << std::endl;
    }

    // Wait for threads to count down
    ctx.stop_latch.wait();

    for (auto &t : worker_threads) {
      if (t.joinable()) {
        t.join();
      }
    }
  }
};