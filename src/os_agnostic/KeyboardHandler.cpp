#include "Context.cpp"
#include <optional>
#include <future>
#include "../os_dependent/ScannerLibrary.cpp"
#include <iostream>
#include <string>
#include <chrono>
#pragma once

class KeyboardHandler : public Handler
{
public:
  KeyboardHandler(MarqueeContext& c) : Handler(c) {}

  void operator()()
  {
    std::cout << "... Keyboard Handler is waiting." << std::endl;
    this->ctx.phase_barrier.arrive_and_wait();
    std::cout << "... Keyboard Handler is starting." << std::endl;

    while (true)
    {
      auto ch = CrossPlatformChar::readCharFor(slice);
      
      if (!ch) continue;
      
      std::cout << "\"" << partialBuffer << "\"" << std::endl;

      switch (*ch)
      {
      case '\r':
      case '\n':
  
        if (commandHandlerCallback) {
          std::cout << "Callback Present" << std::endl;
          commandHandlerCallback(partialBuffer);
        }

        partialBuffer.clear();
        break;
      case 127:
        if (!partialBuffer.empty())
        {
          partialBuffer.pop_back();
        }
        break;
      default:
        partialBuffer.push_back(*ch);
        break;
      }

      std::this_thread::sleep_for(slice);
    }
  };
  
  void connectHandler(std::function<void(const std::string&)> callbackF){
    std::cout << "... Connecting Handler" << std::endl;
    commandHandlerCallback = std::move(callbackF);    
  };

private:
  std::string partialBuffer;
  const std::chrono::duration<int64_t, std::milli> slice = std::chrono::milliseconds(50); // input gets 0.05s
  bool running = false;
  std::string getUserInput();
  std::function<void(const std::string&)> commandHandlerCallback;
};
