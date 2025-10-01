/**
 * Parses and executes commands with deterministic paint order:
 * 
 *   > prev command
 *   prev feedback
 *   
 *   > current command
 *   > current feedback
 *   marquee
 *   > new prompt
 */

#include "CommandHandler.hpp"
#include <sstream>
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <functional>
#include <cctype>

static void toLowerInPlace(std::string& s) {
  for (auto& ch : s) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
}

void CommandHandler::enqueue(std::string cmd) {
  {
    std::lock_guard<std::mutex> lk(queueMutex);
    commandQueue.push(std::move(cmd));
  }
  queueCv.notify_one();
}

// --- Internal: write the help block assuming coutMutex is already held ---
static void writeHelpUnlocked(std::ostream& os) {
  os << "Commands:\n"
     << "  help                              - displays the commands and its description\n"
     << "  start_marquee                     - starts the marquee animation\n"
     << "  stop_marquee                      - stops the marquee animation\n"
     << "  set_text <text>                   - sets the marquee text\n"
     << "  set_speed <ms>                    - sets the refresh in milliseconds\n"
     << "  load_file <path>                  - (extra) loads ASCII file into marquee text\n"
     << "  exit                              - terminates the console\n";
}

// --- Internal: paint transaction to enforce exact line order and clear old marquee ---
static void paintEchoFeedbackMarqueePrompt(
    MarqueeContext& ctx,
    const std::string& enteredLine,
    const std::function<void(std::ostream&)>& feedbackWriter)
{
  // Snapshot marquee text once (avoid holding textMutex during long prints).
  std::string marqueeNow;
  {
    std::lock_guard<std::mutex> g(ctx.textMutex);
    marqueeNow = ctx.marqueeText;
  }
  const bool showMarquee = ctx.isMarqueeActive();

  // Atomic paint under cout mutex: CLEAR OLD MARQUEE -> echo cmd -> feedback -> (maybe) NEW marquee -> NEW prompt+anchor
  std::lock_guard<std::mutex> lock(ctx.coutMutex);

  // Always position relative to the *current* prompt anchor
  std::cout << "\x1b[u";                       // restore to prompt anchor

  // 1) CLEAR the previous marquee line (one line above the prompt)
  std::cout << "\x1b[1F"                       // move to marquee line
            << "\r\x1b[2K";                    // clear entire line

  // 2) Echo the entered command on the prompt line, then newline into history
  std::cout << "\x1b[u"                        // back to prompt anchor
            << "\r\x1b[2K> " << enteredLine    // clear prompt + echo
            << "\n";

  // 3) Feedback block (can be multi-line; writer prints trailing newlines)
  if (feedbackWriter) feedbackWriter(std::cout);

  // 4) NEW marquee line: only print text if marquee is active; otherwise keep it blank
  if (showMarquee) {
    std::cout << "\x1b[2K" << marqueeNow << "\n";
  } else {
    std::cout << "\x1b[2K" << "\n";            // blank marquee line (reserve the slot)
  }

  // 5) NEW prompt line and save a fresh anchor for Display/Keyboard
  std::cout << "\x1b[2K> "                     // prompt
            << "\x1b[s"                        // save NEW prompt anchor
            << std::flush;

  ctx.setHasPromptLine(true);
}

// Fallback one-line message using the same atomic repaint
static void paintMessage(MarqueeContext& ctx, const std::string& enteredLine, const std::string& msg) {
  paintEchoFeedbackMarqueePrompt(ctx, enteredLine, [&](std::ostream& os){
    os << msg << "\n";
  });
}

void CommandHandler::printHelp() {
  // Keep for direct calls if needed, but handleCommand now uses paintEcho... to guarantee order.
  std::lock_guard<std::mutex> lock(ctx.coutMutex);
  writeHelpUnlocked(std::cout);
  std::cout.flush();
}

void CommandHandler::handleCommand(const std::string& line) {
  // Parse command + rest
  auto first_space = line.find_first_of(" \t");
  std::string cmd  = (first_space == std::string::npos) ? line : line.substr(0, first_space);
  std::string rest = (first_space == std::string::npos) ? std::string{} : line.substr(first_space + 1);
  toLowerInPlace(cmd);

  // HELP
  if (cmd == "help") {
    paintEchoFeedbackMarqueePrompt(ctx, line, [&](std::ostream& os){
      writeHelpUnlocked(os);
    });
    return;
  }

  // EXIT (no new prompt afterwards)
  if (cmd == "exit") {
    // Paint a final message but don't reprint the prompt after we flip the flag.
    {
      std::lock_guard<std::mutex> lock(ctx.coutMutex);
      std::cout << "\x1b[u"              // prompt anchor
                << "\r\x1b[2K> " << line << "\n"
                << "Exiting...\n"
                << std::flush;
    }
    ctx.exitRequested.store(true);
    queueCv.notify_all();
    return;
  }

  // START
  if (cmd == "start_marquee") {
    if (display) display->start();
    ctx.runHandler();
    paintMessage(ctx, line, "Marquee started.");
    return;
  }

  // STOP
  if (cmd == "stop_marquee") {
    if (display) display->stop();
    ctx.pauseHandler();
    paintMessage(ctx, line, "Marquee stopped.");
    return;
  }

  // SET SPEED
  if (cmd == "set_speed") {
    auto trim = [](std::string s){
      auto l = s.find_first_not_of(" \t");
      auto r = s.find_last_not_of(" \t");
      if (l == std::string::npos) return std::string{};
      return s.substr(l, r - l + 1);
    };
    std::string arg = trim(rest);
    int ms = -1;
    if (!arg.empty()) {
      std::istringstream iss(arg);
      iss >> ms;
    }
    if (ms >= 0) {
      if (ms < 10) ms = 10;
      ctx.speedMs.store(ms);
      paintMessage(ctx, line, "Speed set to " + std::to_string(ms) + " ms.");
    } else {
      paintMessage(ctx, line, "Usage: set_speed <ms>");
    }
    return;
  }

  // SET TEXT
  if (cmd == "set_text") {
    auto trimQuotes = [](std::string s){
      auto l = s.find_first_not_of(" \t");
      auto r = s.find_last_not_of(" \t");
      if (l == std::string::npos) return std::string{};
      s = s.substr(l, r - l + 1);
      if (s.size() >= 2 && ((s.front()=='"' && s.back()=='"') || (s.front()=='\'' && s.back()=='\''))) {
        s = s.substr(1, s.size()-2);
      }
      return s;
    };
    std::string txt = trimQuotes(rest);
    ctx.setText(txt);
    paintMessage(ctx, line, "Text updated.");
    return;
  }

  // LOAD FILE (extra)
  if (cmd == "load_file") {
    auto trimQuotes = [](std::string s){
      auto l = s.find_first_not_of(" \t");
      auto r = s.find_last_not_of(" \t");
      if (l == std::string::npos) return std::string{};
      s = s.substr(l, r - l + 1);
      if (s.size() >= 2 && ((s.front()=='"' && s.back()=='"') || (s.front()=='\'' && s.back()=='\''))) {
        s = s.substr(1, s.size()-2);
      }
      return s;
    };
    std::string path = trimQuotes(rest);
    if (!fileReader) {
      paintMessage(ctx, line, "File reader not available.");
      return;
    }
    fileReader->loadASCII(path, [this](const std::string &content){
      if (!content.empty()) {
        ctx.setText(content);
        if (display) display->start();
      }
    });
    paintMessage(ctx, line, "Loaded: " + path);
    return;
  }

  // Legacy aliases (not shown in help)
  if (cmd == "marquee") {
    std::istringstream iss(rest);
    std::string sub; iss >> sub; toLowerInPlace(sub);
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
    paintMessage(ctx, line, "Unknown marquee subcommand. Type 'help'.");
    return;
  }

  if (cmd == "video") {
    // Deprecated → mapped to marquee
    paintMessage(ctx, line, "(note) 'video ...' is deprecated; use start_marquee/stop_marquee/set_speed/set_text");
    std::string mapped = "marquee " + rest;
    handleCommand(mapped);
    return;
  }

  if (cmd == "file") {
    std::istringstream iss(rest);
    std::string sub; iss >> sub; toLowerInPlace(sub);
    if (sub == "load") {
      std::string path; std::getline(iss, path);
      return handleCommand("load_file " + path);
    }
  }

  // Unknown
  paintMessage(ctx, line, "Unknown command. Type 'help'.");
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
