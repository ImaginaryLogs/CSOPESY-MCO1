#include "display.cpp"
#include "scanner.cpp"
#include "contract.cpp"
#include <thread>
#include <mutex>

class MarqueeConsole
{
private:
  std::string videoName;
  ScreenDisplay display;
  InputScanner scanner;
  bool isRunning;
  SyncContext ctx;

public:
  MarqueeConsole()
    : display(ctx),
      scanner(ctx)
  {}

  void runLoop()
  {
    isRunning = true;

    ScreenDisplay display(ctx);
    InputScanner scanner(ctx);
    
    std::thread display_thread(std::ref(display));
    std::thread scanner_thread(std::ref(scanner));


    std::this_thread::sleep_for(std::chrono::seconds(5)); // run for 5s
    stop();

    display_thread.join();
    scanner_thread.join();
  };

  void stop()
  {
    {
      std::lock_guard<std::mutex> lock(ctx.screenLock);
      ctx.running = false;
    }
    ctx.cv.notify_all();
  }
};
