/**

 * @file CommandHandler.cpp
 * @brief The command runner's implementation.

 *
 * Objective: maintain a consistent console layout to prevent lines from becoming confused with the
 * marquee.  We perform "atomic" prints by constantly moving and locking ctx.coutMutex.
 * in relation to a "prompt anchor" that has been saved.
 *
 * The arrangement surrounding the prompt appears as follows:
 *
 * > the earlier command
 * Prior comments
 *
 * > present command
 * Current feedback, which may consist of several lines
 * marquee (one line; this is updated above the prompt by the display thread)
 * > new prompt
 *
 * We consistently:
 * 1) return to the prompt anchor,
 * 2) remove the previous marquee line, which is located directly above the prompt.
 * echo the command that was entered,
 * 4) Print the comments,
 * 5) Print a new marquee line (snapshot or blank).
 * 6) Save a new anchor and print a fresh prompt.
 */

#include "CommandHandler.hpp"
#include <sstream>
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <functional>
#include <cctype>

/**
 * @brief Safely lowercase a string (manages signed characters).
 * @param s String for in-place modification.
 */
static void toLowerInPlace(std::string& s) {
  for (auto& ch : s) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
}

/**
 * @brief Add a command to the consumer loop's queue.
 * @param cmd Enqueue command line.
 */
void CommandHandler::enqueue(std::string cmd) {
  {
    std::lock_guard<std::mutex> lk(queueMutex);
    commandQueue.push(std::move(cmd));
  }
  queueCv.notify_one();
}

/**
 * The help menu is printed by the @brief Helper. The caller must already have coutMutex.
 * @param os Output stream (often std::cout).
 */
static void writeHelpUnlocked(std::ostream& os) {
  os << "Commands:\n"
     << "  help                              - shows the commands and their descriptions\n"
     << "  start_marquee                     - starts the animation of the marquee\n"
     << "  stop_marquee                      - stops the animation of the marquee\n"
     << "  set_text <text>                   - sets the text of the marquee\n"
     << "  set_speed <ms>                    - sets the refresh rate in milliseconds\n"
     << "  load_file <path>                  - (extra) loads ASCII file into marquee text\n"
     << "  exit                              - exits the program\n";
}

/**
 * @brief Paint one console output "transaction" so that lines show up in the correct order.
 *
 * Procedures (all with ctx.coutMutex held):
 * - remove the previous marquee line, which is one row above the prompt.
 * - echo the command line (>...),
 * - print feedback (up to several lines may be printed by the writer),
 * - print one marquee line (a snapshot or a blank one),
 * - save a new anchor and print a new prompt.
 *
 * @param ctx Shared context (shared state and sync).
 * @param enteredLine The typed command (which we echo).
 * @param feedbackWriter Function that adds comments to the stream. It might be empty.
 */
static void paintEchoFeedbackMarqueePrompt(
    MarqueeContext& ctx,
    const std::string& enteredLine,
    const std::function<void(std::ostream&)>& feedbackWriter)
{
  // Grab a snapshot of the text on the marquee (short lock to avoid blocking long prints).
  std::string marqueeNow;
  {
    std::lock_guard<std::mutex> g(ctx.textMutex);
    marqueeNow = ctx.marqueeText;
  }
  const bool showMarquee = ctx.isMarqueeActive();

  // Ensure that nothing interferes with the display output, there is a large critical section.
  std::lock_guard<std::mutex> lock(ctx.coutMutex);

  // Always follows the prompt anchor that has been saved.
  std::cout << "\x1b[u";                       // back to prompt anchor

  // (1) Removes the previous marquee line, which is located directly above the prompt.
  std::cout << "\x1b[1F"                       // go up 1 row
            << "\r\x1b[2K";                    // clear the row

  // (2) The command that was entered is echoed on the prompt line.
  std::cout << "\x1b[u"                        // back to prompt
            << "\r\x1b[2K> " << enteredLine    // print "> command"
            << "\n";

  // (3) Comments (may be more than one line). Lines should be ended with '\n'.
  if (feedbackWriter) feedbackWriter(std::cout);

  // (4) New marquee line. To maintain consistency in layout, leave it blank if it's not running.
  if (showMarquee) {
    std::cout << "\x1b[2K" << marqueeNow << "\n";
  } else {
    std::cout << "\x1b[2K" << "\n";
  }

  // (5) Create a new prompt and save a new anchor so that it can be targeted by the keyboard or display.
  std::cout << "\x1b[2K> "
            << "\x1b[s"                        // save new prompt anchor
            << std::flush;

  ctx.setHasPromptLine(true);
}

/**
 * @brief Use the paint sequence to print a single feedback line with this tiny helper.
 * Shared context with @param ctx.
 * @param enteredLine The command that we are responding to.
 * @param msg One-line message; we include the newline at the end.
 */ 
static void paintMessage(MarqueeContext& ctx, const std::string& enteredLine, const std::string& msg) {
  paintEchoFeedbackMarqueePrompt(ctx, enteredLine, [&](std::ostream& os){
    os << msg << "\n";
  });
}

/**
 * @brief Print the thread-safe help menu whenever needed.
 */
void CommandHandler::printHelp() {
  std::lock_guard<std::mutex> lock(ctx.coutMutex);
  writeHelpUnlocked(std::cout);
  std::cout.flush();
}

/**
 * @brief Call the appropriate action after parsing one line.
 *
 * Flow:
 * - divide the command word from the remaining words,
 * - lowercase the command,
 * - match it and execute the action,
 * To ensure that the output does not conflict with the marquee, draw it using the Atomic Paint Helper.
 *
 * @param line The user's entire input line.
*/
void CommandHandler::handleCommand(const std::string& line) {
  // Split "cmd args"
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

  // EXIT (after this, we don’t print a new prompt)
  if (cmd == "exit") {
    {
      std::lock_guard<std::mutex> lock(ctx.coutMutex);
      std::cout << "\x1b[u"
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

  // Legacy aliases (kept for compatibility)
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
    // Old alias -> forward to the marquee form
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

  // Unknown command
  paintMessage(ctx, line, "Unknown command. Type 'help'.");
}

/**
 * @brief Waits for commands and runs them until we’re told to exit.
 *
 * Waiting logic:
 *  - If the queue is empty, we wait on queueCv.
 *  - We wake up when someone enqueues a command or when exit is requested.
 *  - We pop one command at a time and pass it to handleCommand().
 */
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
