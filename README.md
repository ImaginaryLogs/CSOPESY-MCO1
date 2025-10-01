# Marquee Console <!-- omit from toc -->

<!-- ![title](./assets/readme/title.jpg) -->

<!-- Refer to https://shields.io/badges for usage -->

![Year, Term, Course](https://img.shields.io/badge/AY2526--T1-CSOPESY-blue) ![C++](https://img.shields.io/badge/C++-%2300599C.svg?logo=c%2B%2B&logoColor=white)

A command-line marquee console application that displays scrolling text with customizable speed and controls. Built with threading, synchronization primitives, and cross-platform compatibility.

Made by the following students of CSOPESY S13:

- Christian Joseph C. Bunyi
- Enzo Rafael S. Chan
- Roan Cedric V. Campo
- Mariella Jeanne A. Dellosa

The main function can be found in `src/main.cpp`. Jump to [2. Building (CMake presets)](#2-building-cmake-presets) for instructions on how to run the program.

## Table of Contents <!-- omit from toc -->

- [1. Overview](#1-overview)
  - [1.1. Console UI](#11-console-ui)
  - [1.2. Command interpreter](#12-command-interpreter)
  - [1.3. Display handler](#13-display-handler)
  - [1.4. Keyboard handler](#14-keyboard-handler)
  - [1.5. Marquee animation logic](#15-marquee-animation-logic)
- [2. Building](#2-building)
- [3. Running](#3-running)
  - [3.1. Windows (VS 2022 Developer Command Prompt)](#31-windows-vs-2022-developer-command-prompt)
  - [3.2. Linux/macOS/WSL](#32-linuxmacoswsl)
- [4. Usage](#4-usage)
  - [4.1. Commands](#41-commands)
  - [4.2. Demo](#42-demo)

## 1. Overview

The Marquee Console is a simple multithreaded program that shows scrolling text while letting users type commands. It demonstrates basic operating system concepts like threads working together and sharing data safely.

**How it works:**
- **Multiple Threads**: The program uses 4 separate threads - one for display, one for keyboard input, one for processing commands, and one supervisor thread
- **Shared Memory**: All threads share the same data (like the text to display and animation speed) but use locks to prevent conflicts
- **Thread Safety**: Uses mutexes (locks) to make sure only one thread can change shared data at a time
- **Communication**: Threads talk to each other through shared variables and message queues

```cpp
// Shared data structure that all threads can access
struct MarqueeContext {
    std::mutex coutMutex;           // Lock for printing to screen
    std::mutex textMutex;           // Lock for changing marquee text
    std::atomic<bool> exitRequested{false};  // Flag to tell all threads to stop
    std::atomic<int> speedMs{200};  // How fast the animation runs
    std::string marqueeText;        // The text being displayed
};
```

This design lets the user type commands while the text keeps scrolling smoothly in the background.

### 1.1. Console UI

The `MarqueeConsole` class is like the "manager" of the whole program. It starts up all the different threads and makes sure they can talk to each other properly.

**What it does:**
- **Starts Threads**: Creates 4 worker threads when the program begins
- **Connects Components**: Links the keyboard input to the command processor, and the command processor to the display
- **Manages Shutdown**: Makes sure all threads stop cleanly when the program exits

```cpp
class MarqueeConsole {
private:
    MarqueeContext ctx;        // Shared data for all threads
    DisplayHandler display;    // Handles the scrolling animation
    KeyboardHandler keyboard;  // Reads what the user types
    CommandHandler command;    // Processes user commands
};
```

**How components connect:**
```cpp
// In the constructor, we connect the pieces:
MarqueeConsole::MarqueeConsole() {
    // When user types a command, send it to the command handler
    keyboard.setSink([this](std::string cmd) {
        command.enqueue(cmd);
    });
    
    // Let the command handler control the display
    command.addDisplayHandler(&display);
}
```

Think of it like a factory assembly line - each worker (thread) has a specific job, and the console manager coordinates them all.

### 1.2. Command interpreter

The `CommandHandler` processes user commands using a simple queue system. When you type a command and press Enter, it gets added to a queue, and the command handler processes them one by one in the order they were typed.

**What it does:**
- **Receives commands** through a thread-safe queue
- **Parses command strings** to understand what the user wants
- **Controls the display** (start/stop_marquee)
- **Updates text and speed settings** (set_text/speed)
- **Loads files** when requested

**How it works:**
- **Command Queue**: Uses a FIFO (First In, First Out) queue to store commands
- **Thread Safety**: Uses a mutex (lock) so multiple threads can't mess up the queue at the same time
- **Waiting**: When there are no commands, the thread goes to sleep until a new command arrives
- **Processing**: Takes commands from the queue and figures out what to do with them

```cpp
class CommandHandler {
private:
    std::queue<std::string> commandQueue;  // Stores commands waiting to be processed
    std::mutex queueMutex;                 // Lock to protect the queue
    std::condition_variable queueCv;       // Wakes up the thread when new commands arrive
};
```

**Adding a command to the queue:**
```cpp
void CommandHandler::enqueue(std::string cmd) {
    // Lock the queue, add the command, then notify the worker
    std::lock_guard<std::mutex> lock(queueMutex);
    commandQueue.push(cmd);
    queueCv.notify_one();  // Wake up the sleeping command processor
}
```

**Processing commands:**
```cpp
void CommandHandler::operator()() {
    while (program_is_running) {
        // Wait for a command to arrive
        wait_for_command();
        
        // Take the next command and process it
        std::string cmd = commandQueue.front();
        commandQueue.pop();
        handleCommand(cmd);  // Do what the command says
    }
}
```

**Available Commands:**
- `help` — Show all available commands
- `start_marquee` — Start the scrolling text animation
- `stop_marquee` — Stop the animation
- `set_text <message>` — Change what text is displayed
- `set_speed <number>` — Change how fast the text scrolls
- `exit` — Close the program

### 1.3. Display handler

The `DisplayHandler` creates the scrolling text animation. It runs in its own thread and continuously updates what you see on screen while making sure it doesn't interfere with your typing.

**What it does:**
- **Animation Loop**: Continuously moves the text one character to the left to create scrolling effect
- **Thread Safety**: Uses locks to make sure the display doesn't conflict with user input
- **Smart Printing**: Knows where the user is typing and displays the marquee above it
- **Speed Control**: Checks the animation speed setting each time it updates

```cpp
class DisplayHandler {
public:
    void start() { /* Turn on the animation */ }
    void stop()  { /* Turn off the animation */ }
    /* - */
}
```


**The main animation loop:**
```cpp
void DisplayHandler::operator()() {
    while (program_is_running) {
        if (animation_is_on) {
            // Get the current text safely
            std::string current_text = ctx.getText();
            
            // Scroll it one position
            current_text = scrollOnce(current_text);
            
            // Save it back
            ctx.setText(current_text);
            
            // Display it on screen (with proper locking)
            {
                std::lock_guard<std::mutex> lock(ctx.coutMutex);
                std::cout << current_text << std::flush;
            }
        }
        
        // Wait before next frame (speed control)
        std::this_thread::sleep_for(std::chrono::milliseconds(animation_speed));
    }
}
```

**Key Features:**
- **Shared Data**: All threads can access the same marquee text and animation settings
- **Atomic Reads**: Speed changes are read instantly without stopping the animation
- **Mutex Protection**: Uses locks to prevent multiple threads from printing at the same time
- **Immediate Updates**: Text and speed changes take effect on the very next animation frame

### 1.4. Keyboard handler

The keyboard handler runs in its own thread and collects keystrokes into a buffer with basic editing.

When the user presses Enter, it submits the buffered line immediately to the command interpreter through an injected sink callback.

It remains responsive by using non-blocking polling through a platform-specific scanner (Windows uses `_kbhit/_getch`, POSIX uses raw `termios` + `select()` + `read()`).

Moreover, it aims to keep the prompt clean by allocating a cursor anchor and redrawing the buffer under a console mutex so typing never interleaves with marquee output.

```21:48:src/os_agnostic/KeyboardHandler.hpp
class KeyboardHandler : public Handler {
public:
    explicit KeyboardHandler(MarqueeContext& c) : Handler(c) {}
    void operator()();
    void setSink(std::function<void(std::string)> sink) {
        deliver = std::move(sink);
    }
private:
    std::function<void(std::string)> deliver;
};
```

As a design consideration, we inject a sink rather than hard-wiring a dependency, which keeps input collection separated from command processing, thereby simplifying testing.

```17:27:src/os_agnostic/KeyboardHandler.cpp
static void ensurePromptAnchor(MarqueeContext& ctx) {
    if (!ctx.getHasPromptLine()) {
        std::lock_guard<std::mutex> lock(ctx.coutMutex);
        std::cout << "\n\n" << "> " << "\x1b[s" << std::flush;
        ctx.setHasPromptLine(true);
    }
}
```

This anchor reserves space for status and marquee above the prompt and lets us redraw.

```39:49:src/os_agnostic/KeyboardHandler.cpp
static void redrawPrompt(MarqueeContext& ctx, const std::string& buf) {
    std::lock_guard<std::mutex> lock(ctx.coutMutex);
    std::cout << "\x1b[u" << "\r\x1b[2K> " << buf << "\x1b[s" << std::flush;
    ctx.setHasPromptLine(true);
}
```

Redrawing clears the line, restores to the anchor, prints the prompt and buffer, then saves the new end-of-line anchor.

```59:86:src/os_agnostic/KeyboardHandler.cpp
void KeyboardHandler::operator()() {
    ctx.phase_barrier.arrive_and_wait();
    Scanner scan; std::string buffer;
    ensurePromptAnchor(ctx);
    while (!ctx.exitRequested.load()) {
        if (!ctx.getHasPromptLine()) { ensurePromptAnchor(ctx); }
        int ch = scan.poll(); if (ch < 0) continue;
        if (ch == '\n') { const std::string submitted = buffer; buffer.clear(); if (deliver) deliver(submitted);
        } else if (ch == 3) { ctx.exitRequested.store(true); break;
        } else if (ch == 127 || ch == 8) { if (!buffer.empty()) { buffer.pop_back(); redrawPrompt(ctx, buffer); }
        } else if (ch >= 32 && ch < 127) { buffer.push_back(static_cast<char>(ch)); redrawPrompt(ctx, buffer); }
    }
}
```

The main loop polls without blocking, handles Enter for submission, Ctrl+C for shutdown, Backspace for editing, and printable ASCII for input.

Notably, the platform-specific input is abstracted behind `Scanner`, which maps to `src/os_dependent/Scanner_win32.cpp` on Windows and `src/os_dependent/Scanner_posix.cpp` on POSIX so the handler logic remains portable while preserving non-blocking.

### 1.5. Marquee animation logic

The marquee renderer animates a smooth single-line scroll by rotating the text one character at a time and redrawing on a single console line just above the prompt.

It prints according to a configured refresh interval and active state from shared context and avoids tearing by updating text under a mutex and guarding redraws with a console mutex.

Before entering the loop, all handlers synchronize via a barrier and the renderer enables Windows virtual terminal processing so ANSI cursor commands work consistently across platforms (on POSIX, this helper is a no-op).

```17:50:src/os_agnostic/DisplayHandler.hpp
class DisplayHandler : public Handler {
public:
    explicit DisplayHandler(MarqueeContext& c) : Handler(c) {}
    void operator()();
    void start() { ctx.setMarqueeActive(true); }
    void stop()  { ctx.setMarqueeActive(false); }
private:
    std::string scrollOnce(const std::string& s);
};
```

These small surface methods (`start/stop`) flip context flags while the render loop coordinates lifecycle with the barrier at start and a latch countdown on exit.

```38:42:src/os_agnostic/DisplayHandler.cpp
std::string DisplayHandler::scrollOnce(const std::string& s) {
    if (s.empty()) return s;
    std::string out = s.substr(1) + s.substr(0, 1);
    return out;
}
```

The scroll primitive essentially implements a simple “rotate-first-char-to-end” behavior for marquee movement.

```71:90:src/os_agnostic/DisplayHandler.cpp
{
    std::lock_guard<std::mutex> lock(ctx.coutMutex);
    if (ctx.getHasPromptLine()) {
        std::cout << "\x1b[u" << "\x1b[1F" << "\r\x1b[2K" << cur << "\x1b[u" << std::flush;
    } else {
        std::cout << "\r\x1b[2K" << cur << std::flush;
    }
}
std::this_thread::sleep_for(std::chrono::milliseconds(ctx.speedMs.load()));
```

When a prompt is present, we temporarily restore the cursor and draw a frame above it, then restore the prompt’s position; otherwise we draw inline.

Cursor save/restore is what lets the user keep typing without the marquee stealing focus, and the sleep uses `speedMs` directly so `set_speed` takes effect on the next tick.

We decided to read `speedMs` atomically on each iteration to make speed changes immediately visible without restarting the loop. We also decided to use distinct `textMutex` and `coutMutex` mutexes to prevent contention between string rotation and console I/O.

Only console messages are serialized to avoid interleaving with other output.

<!-- ## 2. Prerequisites

### 2.1. Windows (VS 2022 Developer Command Prompt)

- Visual Studio 2022 with C++ development tools
- Windows SDK
- Developer Command Prompt for VS 2022

### 2.2. Linux/macOS/WSL

- GCC 11+ or Clang 14+ with C++23 support
- Make utility
- pthread library -->

## 2. Building

The program utilizes CMake and has to be built in the x64 Native Tools Command Prompt.

```sh
cmake --preset default --fresh
cmake --build --preset default
```

## 3. Running

### 3.1. Windows (VS 2022 Developer Command Prompt)

```bat
bin\app.exe
```

### 3.2. Linux/macOS/WSL

```bash
./bin/app
```

## 4. Usage

### 4.1. Commands

**Required Commands (per PDF spec):**

- `help` — displays the commands and its description
- `start_marquee` — starts the marquee animation
- `stop_marquee` — stops the marquee animation
- `set_text <text>` — sets marquee text
- `set_speed <ms>` — sets refresh in milliseconds
- `exit` — terminates the console

### 4.2. Demo

1. Run the application
2. Use `set_text Hello World` to set the marquee text
3. Use `set_speed 100` to set the animation speed (100ms refresh)
4. Use `start_marquee` to begin the animation
5. Use `stop_marquee` to pause the animation
6. Use `exit` to terminate the application
