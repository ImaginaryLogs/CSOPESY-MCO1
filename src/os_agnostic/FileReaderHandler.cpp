#include "Context.cpp"
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

#pragma once

class FileReaderHandler : public Handler {
public:
  FileReaderHandler(MarqueeContext &c) : Handler(c) {}

  void operator()() {
    std::cout << "... File Reader Handler is waiting." << std::endl;
    this->ctx.phase_barrier.arrive_and_wait();
    std::cout << "... File Reader Handler is starting." << std::endl;

    auto predicateHasRequest = [this] { return !fileRequestQueue.empty(); };

    while (true) {
      std::unique_lock<std::mutex> lock(requestMutex);
      cv.wait(lock, predicateHasRequest);

      while (!fileRequestQueue.empty()) {
        FileRequest request = fileRequestQueue.front();
        fileRequestQueue.pop();
        lock.unlock();

        // Process the file request
        std::cout << "File Reader processing: " << request.filename
                  << std::endl;

        if (request.type == FileRequestType::LOAD_ASCII) {
          loadAsciiFile(request.filename, request.callback);
        }

        lock.lock();
      }
    }
  }

  void requestLoadAscii(const std::string &filename,
                        std::function<void(const std::string &)> callback) {
    std::lock_guard<std::mutex> lock(requestMutex);
    fileRequestQueue.push({filename, FileRequestType::LOAD_ASCII, callback});
    cv.notify_one();
  }

private:
  enum class FileRequestType { LOAD_ASCII };

  struct FileRequest {
    std::string filename;
    FileRequestType type;
    std::function<void(const std::string &)> callback;
  };

  std::queue<FileRequest> fileRequestQueue;
  std::mutex requestMutex;
  std::condition_variable cv;

  void loadAsciiFile(const std::string &filename,
                     std::function<void(const std::string &)> callback) {
    try {
      std::ifstream file(filename);
      if (!file.is_open()) {
        std::string error = "Error: Could not open file '" + filename + "'";
        std::cout << error << std::endl;
        if (callback)
          callback(error);
        return;
      }

      std::string content;
      std::string line;

      // Read the entire file content
      while (std::getline(file, line)) {
        content += line + "\n";
      }

      file.close();

      std::cout << "Successfully loaded ASCII file: " << filename << " ("
                << content.length() << " chars)" << std::endl;

      if (callback)
        callback(content);

    } catch (const std::exception &e) {
      std::string error = "Error reading file '" + filename + "': " + e.what();
      std::cout << error << std::endl;
      if (callback)
        callback(error);
    }
  }
};
