#include "Context.cpp"
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <vector>
#include <functional>   // <-- fix: was <functiona>

#pragma once

class FileReaderHandler : public Handler {
public:
  FileReaderHandler(MarqueeContext &c) : Handler(c) {}

  void operator()() {
    {
      std::lock_guard<std::mutex> lock(ctx.coutMutex);
      std::cout << "... File Reader Handler is waiting." << std::endl;
    }
    ctx.phase_barrier.arrive_and_wait();

    // Passive thread; operations are synchronous per call.
    while (!ctx.exitRequested.load()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    ctx.stop_latch.count_down();
  }

  void loadASCII(const std::string &filename,
                 std::function<void(const std::string &)> callback) {
    try {
      namespace fs = std::filesystem;
      fs::path p = filename;
      if (!p.is_absolute()) {
        fs::path base = fs::current_path();
        if (fs::exists("assets")) base = base / "assets";
        p = base / filename;
      }
      std::ifstream file(p, std::ios::in | std::ios::binary);
      if (!file) {
        std::string msg = "Cannot open file: " + p.string();
        std::cout << msg << std::endl;
        if (callback) callback(msg);
        return;
      }
      std::string content((std::istreambuf_iterator<char>(file)),
                          std::istreambuf_iterator<char>());
      std::cout << "Successfully loaded ASCII file: " << filename << " ("
                << content.length() << " chars)" << std::endl;

      if (callback)
        callback(content);

    } catch (const std::exception &e) {
      std::string error = "Error reading file '" + filename + "': " + std::string(e.what());
      std::cout << error << std::endl;
      if (callback)
        callback(error);
    }
  }
};
