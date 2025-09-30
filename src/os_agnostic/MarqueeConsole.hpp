/**
 * High-level console wiring: starts threads and coordinates shutdown.
 */

#pragma once

#include "Context.hpp"
#include "DisplayHandler.hpp"
#include "KeyboardHandler.hpp"
#include "CommandHandler.hpp"
#include "FileReaderHandler.hpp"
#include <thread>
#include <vector>

class MarqueeConsole {
public:
  MarqueeConsole();
  void run();

private:
  MarqueeContext ctx;
  DisplayHandler display;
  KeyboardHandler keyboard;
  CommandHandler command;
  FileReaderHandler fileReader;
  std::vector<std::thread> threads;
};
