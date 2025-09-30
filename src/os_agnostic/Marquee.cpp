#include "CommandHandler.cpp"
#include "Context.cpp"
#include "FileReaderHandler.cpp"
#include "KeyboardHandler.cpp"
#include <mutex>
#include <thread>

class MarqueeConsole {
private:
  std::string videoName;
  DisplayHandler display;
  KeyboardHandler scanner;
  CommandHandler commandProcessor;
  std::atomic<bool> isRunning;
  MarqueeContext ctx;

public:
  MarqueeConsole() : display(ctx), scanner(ctx), commandProcessor(ctx) {}

  ~MarqueeConsole() { stop(); }

  void run() {
    std::cout << "Running..." << std::endl;
    isRunning = true;
    CommandHandler commander(ctx);
    DisplayHandler display(ctx);
    KeyboardHandler scanner(ctx);
    FileReaderHandler fileReader(ctx);

    scanner.connectHandler(
        [&commander](const std::string &input) { commander.addInput(input); });
    commander.addDisplayHandler(&display);
    commander.addFileReaderHandler(&fileReader);

    std::thread command_thread(std::ref(commander));
    std::thread scanner_thread(std::ref(scanner));
    std::thread display_thread(std::ref(display));
    std::thread file_reader_thread(std::ref(fileReader));

    std::thread file_writer_thread;

    worker_threads.emplace_back(std::move(display_thread));
    worker_threads.emplace_back(std::move(scanner_thread));
    worker_threads.emplace_back(std::move(command_thread));
    worker_threads.emplace_back(std::move(file_reader_thread));

    ctx.handlers_done.wait();

    stop();
  };

private:
  std::vector<std::thread> worker_threads;

  void stop() {

    std::cout << "Exiting..." << std::endl;

    for (auto &t : worker_threads) {
      if (t.joinable()) {
        t.join();
      }
    }
  }
};
