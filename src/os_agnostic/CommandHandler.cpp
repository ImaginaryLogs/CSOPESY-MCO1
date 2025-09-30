#include "../os_dependent/ScannerLibrary.cpp"
#include "Context.cpp"
#include "DisplayHandler.cpp"
#include "FileReaderHandler.cpp"
#include <chrono>
#include <condition_variable>
#include <future>
#include <iostream>
#include <optional>
#include <queue>
#include <string>

#pragma once

class CommandHandler : public Handler {
public:
  CommandHandler(MarqueeContext &c) : Handler(c) {}

  void operator()() {
    std::cout << "... Command Handler is waiting." << std::endl;
    this->ctx.phase_barrier.arrive_and_wait();
    std::cout << "... Command Handler is starting." << std::endl;

    auto predicateHasCommand = [this] { return !commandQueue.empty(); };

    while (true) {
      std::unique_lock<std::mutex> lock(queueMutex);

      cv.wait(lock, predicateHasCommand);
      while (!commandQueue.empty()) {

        std::string command = commandQueue.front();
        commandQueue.pop();

        // Process the command
        std::cout << "Processing command: " << command << std::endl;
        auto args = parseArgs(command);

        if (args[0] == "video") {

          if (args.size() < 2) {
            std::cout << "Missing video subcommand" << std::endl;
          } else if (args[1] == "ping") {
            this->display->ping();
          } else if (args[1] == "display") {
            if (args.size() < 3) {
              std::cout << "Missing argument for video display" << std::endl;
            } else {
              this->display->displayString(args[2]);
            }
          } else if (args[1] == "start") {
            this->display->startVideo();
          } else if (args[1] == "pause") {
            this->display->stopVideo();
          } else {
            std::cout << "Not recognized video command" << std::endl;
          }

        } else if (args[0] == "load") {
          if (args.size() < 2) {
            std::cout << "Missing filename for load command" << std::endl;
          } else {
            std::cout << "Loading ASCII file: " << args[1] << std::endl;
            if (this->fileReader) {
              this->fileReader->requestLoadAscii(args[1], [this](
                                                              const std::string
                                                                  &content) {
                if (content.substr(0, 5) == "Error") {
                  std::cout << content << std::endl;
                } else {
                  std::cout
                      << "File loaded successfully, setting as current frame"
                      << std::endl;
                  this->display->setVideo(content);
                }
              });
            } else {
              std::cout << "File reader not available" << std::endl;
            }
          }

        } else {
          std::cout << "Not recognized command" << std::endl;
        }
      }
    }
  };

  void addInput(const std::string &input) {
    std::lock_guard<std::mutex> lock(queueMutex);
    std::cout << "Command Handler Received: " << input << std::endl;
    commandQueue.push(input);
    cv.notify_one();
  };

  void addDisplayHandler(DisplayHandler *d) {
    this->display = d;
    this->display->ping();
  }

  void addFileReaderHandler(FileReaderHandler *fr) { this->fileReader = fr; }

private:
  std::queue<std::string> commandQueue;
  std::mutex queueMutex;
  std::condition_variable cv;
  DisplayHandler *display;
  FileReaderHandler *fileReader;

  std::vector<std::string> parseArgs(const std::string &input) {
    std::vector<std::string> args;
    std::istringstream iss(input);
    std::string token;

    while (iss >> std::ws) { // skip whitespace
      if (iss.peek() == '"') {
        iss.get();                     // consume the opening quote
        std::getline(iss, token, '"'); // read until closing quote
        args.push_back(token);
      } else {
        iss >> token;
        args.push_back(token);
      }
    }
    return args;
  }
};
