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
#include <sstream>
#include <vector>
#include <algorithm>

#pragma once

class CommandHandler : public Handler {
public:
  CommandHandler(MarqueeContext &c) : Handler(c) {}

  void operator()() {
    {
      std::lock_guard<std::mutex> lock(ctx.coutMutex);
      std::cout << "... Command Handler is ready." << std::endl;
      printWelcome();
    }

    ctx.phase_barrier.arrive_and_wait();

    running = true;
    while (running && !ctx.exitRequested.load()) {
      std::string command;
      {
        std::unique_lock<std::mutex> lock(queueMutex);
        queueCv.wait(lock, [&] { return !commandQueue.empty() || ctx.exitRequested.load(); });
        if (ctx.exitRequested.load()) break;
        command = std::move(commandQueue.front());
        commandQueue.pop();
      }

      handleCommand(command);
    }

    ctx.stop_latch.count_down();
  }

  void addDisplayHandler(DisplayHandler *d) { display = d; }
  void addFileReaderHandler(FileReaderHandler *f) { fileReader = f; }

  void addInput(const std::string &in) {
    {
      std::lock_guard<std::mutex> lock(queueMutex);
      commandQueue.push(in);
    }
    queueCv.notify_one();
  }

private:
  bool running{false};
  std::mutex queueMutex;
  std::condition_variable queueCv;
  std::queue<std::string> commandQueue;
  DisplayHandler *display{nullptr};
  FileReaderHandler *fileReader{nullptr};

  void printWelcome() {
    std::cout << "--------------------------------------------" << std::endl;
    std::cout << " OS Marquee Emulator - Main Menu" << std::endl;
    std::cout << "--------------------------------------------" << std::endl;
    std::cout << "Type 'help' to see available commands." << std::endl;
  }

  static std::vector<std::string> parseArgs(const std::string &line) {
    std::istringstream iss(line);
    std::vector<std::string> tokens;
    std::string token;

    while (iss >> std::ws) {
      if (iss.peek() == '"') {
        iss.get();
        std::getline(iss, token, '"');
        tokens.push_back(token);
      } else {
        iss >> token;
        tokens.push_back(token);
      }
    }
    return tokens;
  }

  void handleCommand(const std::string &line) {
    auto args = parseArgs(line);
    if (args.empty()) return;

    auto toLower = [](std::string s) {
      std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::tolower(c); });
      return s;
    };

    const std::string cmd = toLower(args[0]);

    if (cmd == "help") {
      printHelp();
      return;
    }
    if (cmd == "exit" || cmd == "quit") {
      ctx.exitRequested.store(true);
      running = false;
      return;
    }
    if (cmd == "video") {
      handleVideo(args);
      return;
    }
    if (cmd == "file") {
      handleFile(args);
      return;
    }

    std::lock_guard<std::mutex> lock(ctx.coutMutex);
    std::cout << "Unknown command: " << args[0] << " (type 'help')" << std::endl;
  }

  void printHelp() {
    std::lock_guard<std::mutex> lock(ctx.coutMutex);
    std::cout << "Commands:" << std::endl;
    std::cout << "  help                          - show this help" << std::endl;
    std::cout << "  exit | quit                   - terminate the console" << std::endl;
    std::cout << "  video ping                    - send a ping to display thread" << std::endl;
    std::cout << "  video start                   - start marquee animation" << std::endl;
    std::cout << "  video stop                    - stop marquee animation" << std::endl;
    std::cout << "  video speed <ms>              - set refresh speed in milliseconds" << std::endl;
    std::cout << "  video display \"Your text\"    - set marquee text" << std::endl;
    std::cout << "  file load <path>              - load ASCII file and display (assets/)" << std::endl;
  }

  void handleVideo(const std::vector<std::string> &args) {
    if (args.size() < 2) {
      std::lock_guard<std::mutex> lock(ctx.coutMutex);
      std::cout << "Missing video subcommand (ping|start|stop|speed|display)" << std::endl;
      return;
    }
    const std::string sub = args[1];
    if (sub == "ping") {
      if (display) display->ping();
    } else if (sub == "start") {
      if (display) display->start();
      ctx.runHandler();
    } else if (sub == "stop") {
      if (display) display->stop();
      ctx.pauseHandler();
    } else if (sub == "speed") {
      if (args.size() < 3) {
        std::lock_guard<std::mutex> lock(ctx.coutMutex);
        std::cout << "Usage: video speed <ms>" << std::endl;
      } else {
        try {
          int ms = std::stoi(args[2]);
          if (ms < 5) ms = 5;
          ctx.speedMs.store(ms);
        } catch (...) {
          std::lock_guard<std::mutex> lock(ctx.coutMutex);
          std::cout << "Invalid number: " << args[2] << std::endl;
        }
      }
    } else if (sub == "display") {
      if (args.size() < 3) {
        std::lock_guard<std::mutex> lock(ctx.coutMutex);
        std::cout << "Usage: video display \"Your message\"" << std::endl;
      } else {
        std::ostringstream oss;
        for (size_t i = 2; i < args.size(); ++i) {
          if (i > 2) oss << ' ';
          oss << args[i];
        }
        // Update marquee text only; do not print another "Video Thread:" line.
        if (display) display->displayString(oss.str());
        ctx.setText(oss.str());
      }
    } else {
      std::lock_guard<std::mutex> lock(ctx.coutMutex);
      std::cout << "Unknown video subcommand: " << sub << std::endl;
    }
  }

  void handleFile(const std::vector<std::string> &args) {
    if (args.size() < 3 || args[1] != "load") {
      std::lock_guard<std::mutex> lock(ctx.coutMutex);
      std::cout << "Usage: file load <path>" << std::endl;
      return;
    }
    const std::string path = args[2];
    if (!fileReader) {
      std::lock_guard<std::mutex> lock(ctx.coutMutex);
      std::cout << "File reader not available." << std::endl;
      return;
    }
    fileReader->loadASCII(path, [this](const std::string &content){
      if (display) display->displayString(content);
      ctx.setText(content);
    });
  }
};