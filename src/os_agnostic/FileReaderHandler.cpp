/**
 * Loads ASCII files and returns content to callbacks.
 */

#include "FileReaderHandler.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

void FileReaderHandler::loadASCII(const std::string& path, std::function<void(const std::string&)> cb) {
  try {
    std::ifstream in(path);
    if (!in) {
      std::lock_guard<std::mutex> lock(ctx.coutMutex);
      std::cout << "Error: cannot open file: " << path << std::endl;
      if (cb) cb(std::string{});
      return;
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    auto content = ss.str();
    if (cb) cb(content);
  } catch (const std::exception& e) {
    std::lock_guard<std::mutex> lock(ctx.coutMutex);
    std::cout << "Error reading file '" << path << "': " << e.what() << std::endl;
    if (cb) cb(std::string{});
  }
}
