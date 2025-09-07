#include "contract.cpp"

class InputScanner
{
public:
  explicit InputScanner(SyncContext& c) : ctx(c) {}

  void operator()() {
    using clock = std::chrono::steady_clock;
    const auto slice = std::chrono::milliseconds(400); // input gets 0.4s

    while (true)
    {
      std::unique_lock<std::mutex> lock(ctx.screenLock);
      ctx.cv.wait(lock, scannerTurnPredicate);
      if (!ctx.running) break;

      

      auto deadline = clock::now() + slice;

      while(clock::now() < deadline && ctx.running)
      {
        lock.unlock();
        std::cout << "Testing Input Scanner\n" << std::flush;

        lock.lock();
        if (!ctx.running) break;
      }

      ctx.turn = SyncContext::Turn::Display;
      lock.unlock();
      ctx.cv.notify_all();
    }
  };
  void processChoice();

private:
  SyncContext &ctx;
  bool running = false;
  std::string getUserInput();
  bool isScannerTurn() { return ctx.turn == SyncContext::Turn::Input || !ctx.running; };
  std::function<bool()> scannerTurnPredicate = [this]()
  { return isScannerTurn(); };
};
