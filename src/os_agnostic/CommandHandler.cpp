

#include "Context.cpp"
#include <optional>
#include <future>
#include "../os_dependent/ScannerLibrary.cpp"
#include <iostream>
#include <string>
#include <queue>
#include <chrono>
#include <condition_variable> 

#pragma once


class CommandHandler : public Handler
{
  public:
  CommandHandler(MarqueeContext& c) : Handler(c) {}

  void operator()()
   {
    std::cout << "... Command Handler is waiting." << std::endl;
    this->ctx.phase_barrier.arrive_and_wait();
    std::cout << "... Command Handler is starting." << std::endl;

    


    auto predicateHasCommand = [this] {
      return !commandQueue.empty();
    };

     while (true)
     {
       std::unique_lock<std::mutex> lock(queueMutex);

       cv.wait(lock, predicateHasCommand);
       while (!commandQueue.empty())
       {
         std::string command = commandQueue.front();
         commandQueue.pop();
         lock.unlock();

         // Process the command
         std::cout << "Processing command: " << command << std::endl;

         lock.lock();
       }
     }
   };

  void addInput(const std::string& input){
    std::lock_guard<std::mutex> lock(queueMutex);
    std::cout << "Command Handler Recieved: " << input << std::endl;
    commandQueue.push(input);
    cv.notify_one();
  };

  

  private:
  std::queue<std::string> commandQueue;
  std::mutex queueMutex;
  std::condition_variable cv;
  
};