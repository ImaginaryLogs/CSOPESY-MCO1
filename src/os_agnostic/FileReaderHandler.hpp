/**
 * Loads ASCII files and returns content to callbacks.
 */

#pragma once
#include "Context.hpp"
#include <functional>
#include <string>

class FileReaderHandler : public Handler {
public:
  explicit FileReaderHandler(MarqueeContext &c) : Handler(c) {}

  void loadASCII(const std::string& path, std::function<void(const std::string&)> cb);
};
