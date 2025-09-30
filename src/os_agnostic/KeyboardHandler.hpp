/**
 * Reads user input and forwards them as commands.
 */

#pragma once
#include "Context.hpp"
#include "../os_dependent/Scanner.hpp"
#include <string>
#include <functional>

class KeyboardHandler : public Handler {
public:
  explicit KeyboardHandler(MarqueeContext& c) : Handler(c) {}

  void operator()();

  // injection point to deliver commands to command processor
  void setSink(std::function<void(std::string)> sink) { deliver = std::move(sink); }

private:
  std::function<void(std::string)> deliver;
};
