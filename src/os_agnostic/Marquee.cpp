#include "DisplayHandler.cpp"
#include "KeyboardHandler.cpp"
#include "Context.cpp"
#include <thread>
#include <mutex>

class MarqueeConsole
{
private:
  std::string videoName;
  ScreenDisplay display;
  InputScanner scanner;
  bool isRunning;
  MarqueeContext ctx;

public:
  MarqueeConsole() : display(ctx), scanner(ctx) {}

  ~MarqueeConsole() { stop(); }

  void runDispatcher()
  {
    isRunning = true;

    ScreenDisplay display(ctx);
    InputScanner scanner(ctx);

    std::thread display_thread(std::ref(display));
    std::thread scanner_thread(std::ref(scanner));

    while (isRunning)

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
