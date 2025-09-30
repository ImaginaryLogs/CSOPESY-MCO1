/**
 * Parses and executes commands.
 */

#include "CommandHandler.hpp"
#include <sstream>
#include <iostream>
#include <algorithm>
#include <iomanip>

void CommandHandler::enqueue(std::string cmd) {
  {
    std::lock_guard<std::mutex> lk(queueMutex);
    commandQueue.push(std::move(cmd));
  }
  queueCv.notify_one();
}

void CommandHandler::printHelp() {
  std::lock_guard<std::mutex> lock(ctx.coutMutex);
  std::cout << "Commands:\n"
            << "  help                              - displays the commands and its description\n"
            << "  start_marquee                     - starts the marquee animation\n"
            << "  stop_marquee                      - stops the marquee animation\n"
            << "  set_text <text>                   - sets the marquee text\n"
            << "  set_speed <ms>                    - sets the refresh in milliseconds\n"
            << "  load_file <path>                  - (extra) loads ASCII file into marquee text\n"
            << "  exit                              - terminates the console\n";
  std::cout.flush();
}

void CommandHandler::handleCommand(const std::string& line) {
  // tokenize: first word is command, remainder is payload
  auto first_space = line.find_first_of(" \t");
  std::string cmd = (first_space == std::string::npos) ? line : line.substr(0, first_space);
  std::string rest = (first_space == std::string::npos) ? std::string{} : line.substr(first_space + 1);
  // normalize command to lowercase
  std::transform(cmd.begin(), cmd.end(), cmd.begin(), [](unsigned char c){ return std::tolower(c); });

  if (cmd == "help") {
    printHelp();
    return;
  }
  if (cmd == "exit") {
    ctx.exitRequested.store(true);
    queueCv.notify_all();
    return;
  }
  if (cmd == "start_marquee") {
    if (display) display->start();
    ctx.runHandler(); // naming preserved
    {
      std::lock_guard<std::mutex> lock(ctx.coutMutex);
      std::cout << "Marquee started.\n";
    }
    return;
  }
  if (cmd == "stop_marquee") {
    if (display) display->stop();
    ctx.pauseHandler();
    {
      std::lock_guard<std::mutex> lock(ctx.coutMutex);
      std::cout << "Marquee stopped.\n";
    }
    return;
  }
  if (cmd == "set_speed") {
    try {
      std::istringstream iss(rest);
      int ms; iss >> ms;
      if (!iss.fail()) {
        if (ms < 10) ms = 10;
        ctx.speedMs.store(ms);
        std::lock_guard<std::mutex> lock(ctx.coutMutex);
        std::cout << "Speed set to " << ms << " ms.\n";
      } else {
        std::lock_guard<std::mutex> lock(ctx.coutMutex);
        std::cout << "Usage: set_speed <ms>\n";
      }
    } catch (...) {
      std::lock_guard<std::mutex> lock(ctx.coutMutex);
      std::cout << "Usage: set_speed <ms>\n";
    }
    return;
  }
  if (cmd == "set_text") {
    auto trim = [](std::string s){
      auto l = s.find_first_not_of(" \t");
      auto r = s.find_last_not_of(" \t");
      if (l == std::string::npos) return std::string{};
      s = s.substr(l, r - l + 1);
      if ((s.size() >= 2) && ((s.front()=='"' && s.back()=='"') || (s.front()=='\'' && s.back()=='\''))) {
        s = s.substr(1, s.size()-2);
      }
      return s;
    };
    std::string txt = trim(rest);
    ctx.setText(txt);
    {
      std::lock_guard<std::mutex> lock(ctx.coutMutex);
      std::cout << "Text updated.\n";
    }
    return;
  }
  if (cmd == "load_file") {
    auto trim = [](std::string s){
      auto l = s.find_first_not_of(" \t");
      auto r = s.find_last_not_of(" \t");
      if (l == std::string::npos) return std::string{};
      s = s.substr(l, r - l + 1);
      if ((s.size() >= 2) && ((s.front()=='"' && s.back()=='"') || (s.front()=='\'' && s.back()=='\''))) {
        s = s.substr(1, s.size()-2);
      }
      return s;
    };
    std::string path = trim(rest);
    if (!fileReader) {
      std::lock_guard<std::mutex> lock(ctx.coutMutex);
      std::cout << "File reader not available.\n";
      return;
    }
    fileReader->loadASCII(path, [this](const std::string &content){
      if (!content.empty()) {
        ctx.setText(content);
        if (display) display->start();
      }
    });
    {
      std::lock_guard<std::mutex> lock(ctx.coutMutex);
      std::cout << "Loaded: " << path << "\n";
    }
    return;
  }

  // Legacy aliases (not in help)
  if (cmd == "marquee") {
    std::istringstream iss(rest);
    std::string sub; iss >> sub;
    std::transform(sub.begin(), sub.end(), sub.begin(), [](unsigned char c){ return std::tolower(c); });
    if (sub == "start") return handleCommand("start_marquee");
    if (sub == "stop")  return handleCommand("stop_marquee");
    if (sub == "speed") {
      std::string ms; iss >> ms;
      return handleCommand("set_speed " + ms);
    }
    if (sub == "text") {
      std::string text;
      std::getline(iss, text);
      return handleCommand("set_text " + text);
    }
    std::lock_guard<std::mutex> lock(ctx.coutMutex);
    std::cout << "Unknown marquee subcommand. Type 'help'.\n";
    return;
  }
  if (cmd == "video") {
    std::lock_guard<std::mutex> lock(ctx.coutMutex);
    std::cout << "(note) 'video ...' is deprecated; use start_marquee/stop_marquee/set_speed/set_text\n";
    std::string mapped = "marquee " + rest;
    handleCommand(mapped);
    return;
  }
  if (cmd == "file") {
    std::istringstream iss(rest);
    std::string sub; iss >> sub;
    std::transform(sub.begin(), sub.end(), sub.begin(), [](unsigned char c){ return std::tolower(c); });
    if (sub == "load") {
      std::string path;
      std::getline(iss, path);
      return handleCommand("load_file " + path);
    }
  }

  // Fallback
  {
    std::lock_guard<std::mutex> lock(ctx.coutMutex);
    std::cout << "Unknown command. Type 'help'.\n";
  }
}

void CommandHandler::operator()() {
  // >>> JOIN INIT PHASE
  ctx.phase_barrier.arrive_and_wait();

  while (!ctx.exitRequested.load()) {
    std::string command;
    {
      std::unique_lock<std::mutex> lock(queueMutex);
      queueCv.wait(lock, [&]{ return !commandQueue.empty() || ctx.exitRequested.load(); });
      if (ctx.exitRequested.load()) break;
      command = std::move(commandQueue.front());
      commandQueue.pop();
    }
    handleCommand(command);
  }

  ctx.stop_latch.count_down();
}
