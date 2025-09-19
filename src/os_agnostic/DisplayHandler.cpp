#include <string>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include "Context.cpp"
#include <functional>

class DisplayHandler : public Handler
{
public:
  DisplayHandler(MarqueeContext& c) : Handler(c) {}

  void operator()()
  {
    std::cout << "... Display Handler is waiting." << std::endl;
    this->ctx.phase_barrier.arrive_and_wait();
    std::cout << "... Display Handler is starting." << std::endl;
    while (true) {
      if (isVideoRunning) {
        clearScreen();
        std::cout << getCurrentFrame() << std::endl;
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(1000 / 60)); // Approx 60 FPS
    }
  };




  bool startVideo()
  {
    isVideoRunning = true;
    return isVideoRunning;
  }

  bool stopVideo()
  {
    isVideoRunning = false;
    return isVideoRunning;
  }

  std::string getCurrentFrame()
  {
    return currentFrame;
  };

  void setVideo(const std::string &videoName)
  {
    currentFrame = videoName;
  };

  void clearScreen()
  {
    std::cout << "\033[2J\033[1;1H";
  };

private:
  std::string currentFrame;
  std::atomic<bool> isVideoRunning{false};
};
