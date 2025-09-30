/**
 * Parses and executes commands from the PDF spec.
 * Supported:
 *   help
 *   exit
 *   start_marquee
 *   stop_marquee
 *   set_speed <ms>
 *   set_text <text>
 * Extra (not in spec, optional):
 *   load_file <path>
 */

#pragma once
#include "Context.hpp"
#include "DisplayHandler.hpp"
#include "FileReaderHandler.hpp"
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

class CommandHandler : public Handler {
public:
  explicit CommandHandler(MarqueeContext& c) : Handler(c) {}

  void operator()(); // consumer loop

  void addDisplayHandler(DisplayHandler* d) { display = d; }
  void addFileReaderHandler(FileReaderHandler* f) { fileReader = f; }

  // external producers call this to enqueue a command
  void enqueue(std::string cmd);

private:
  // queue state (private as requested)
  std::mutex queueMutex;
  std::condition_variable queueCv;
  std::queue<std::string> commandQueue;

  DisplayHandler* display{nullptr};
  FileReaderHandler* fileReader{nullptr};

  // helpers
  void handleCommand(const std::string& line);
  void printHelp();
};
