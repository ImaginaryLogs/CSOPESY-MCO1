/**
 * Renders the marquee frames to the console.
 */

#pragma once
#include "Context.hpp"
#include <atomic>
#include <string>

class DisplayHandler : public Handler {
public:
  explicit DisplayHandler(MarqueeContext &c) : Handler(c) {}

  void operator()();

  // expose control methods used by commands
  void start() { ctx.setMarqueeActive(true); }
  void stop()  { ctx.setMarqueeActive(false); }

private:
  std::string scrollOnce(const std::string& s);
};
