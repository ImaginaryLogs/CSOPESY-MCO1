# Marquee Console <!-- omit from toc -->

<!-- ![title](./assets/readme/title.jpg) -->

<!-- Refer to https://shields.io/badges for usage -->

![Year, Term, Course](https://img.shields.io/badge/AY2526--T1-CSOPESY-blue) ![C++](https://img.shields.io/badge/C++-%2300599C.svg?logo=c%2B%2B&logoColor=white)

A command-line marquee console application that displays scrolling text with customizable speed and controls. Built with modern C++20/23 features including threading, synchronization primitives, and cross-platform compatibility.

## Table of Contents <!-- omit from toc -->

- [1. Overview](#1-overview)
  - [1.1. Console UI](#11-console-ui)
  - [1.2. Command interpreter](#12-command-interpreter)
  - [1.3. Display handler](#13-display-handler)
  - [1.4. Keyboard handler](#14-keyboard-handler)
  - [1.5. Marquee animation logic](#15-marquee-animation-logic)
- [2. Prerequisites](#2-prerequisites)
  - [2.1. Windows (VS 2022 Developer Command Prompt)](#21-windows-vs-2022-developer-command-prompt)
  - [2.2. Linux/macOS/WSL](#22-linuxmacoswsl)
- [3. Building](#3-building)
  - [3.1. Windows (VS 2022 Developer Command Prompt)](#31-windows-vs-2022-developer-command-prompt)
  - [3.2. Linux/macOS/WSL](#32-linuxmacoswsl)
- [4. Running](#4-running)
  - [4.1. Windows (VS 2022 Developer Command Prompt)](#41-windows-vs-2022-developer-command-prompt)
  - [4.2. Linux/macOS/WSL](#42-linuxmacoswsl)
- [5. Usage](#5-usage)
  - [5.1. Commands](#51-commands)
  - [5.2. Demo](#52-demo)
- [6. Technical Notes](#6-technical-notes)

## 1. Overview

The Marquee Console is an OS emulator that provides a command-line interface for displaying scrolling text animations. It consists of several key components working together to create a responsive marquee display system.

> expound on technical stuff needed for the technical report

### 1.1. Console UI

The main console interface that coordinates all components and provides the user interaction layer. It manages the overall application lifecycle and thread coordination.

> expound on technical stuff needed for the technical report

### 1.2. Command interpreter

Handles command parsing and execution. Processes user commands from a thread-safe queue and coordinates with other components to execute marquee operations.

> expound on technical stuff needed for the technical report

### 1.3. Display handler

Manages the visual output and marquee animation rendering. Handles console display updates and text scrolling with customizable refresh rates.

> expound on technical stuff needed for the technical report

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

## 2. Prerequisites

### 2.1. Windows (VS 2022 Developer Command Prompt)

- Visual Studio 2022 with C++ development tools
- Windows SDK
- Developer Command Prompt for VS 2022

### 2.2. Linux/macOS/WSL

- GCC 11+ or Clang 14+ with C++23 support
- Make utility
- pthread library

## 3. Building

### 3.1. Windows (VS 2022 Developer Command Prompt)

```bat
cl /std:c++20 /EHsc src\main.cpp src\os_agnostic\*.cpp src\os_dependent\*.cpp /Fe:bin\app.exe
```

### 3.2. Linux/macOS/WSL

```bash
make
```

## 4. Running

### 4.1. Windows (VS 2022 Developer Command Prompt)

```bat
bin\app.exe
```

### 4.2. Linux/macOS/WSL

```bash
./bin/app
```

## 5. Usage

### 5.1. Commands

**Required Commands (per PDF spec):**

- `help` — displays the commands and its description
- `start_marquee` — starts the marquee animation
- `stop_marquee` — stops the marquee animation
- `set_text <text>` — sets marquee text
- `set_speed <ms>` — sets refresh in milliseconds
- `exit` — terminates the console

### 5.2. Demo

1. Start the application
2. Use `set_text Hello World` to set the marquee text
3. Use `set_speed 100` to set the animation speed (100ms refresh)
4. Use `start_marquee` to begin the animation
5. Use `stop_marquee` to pause the animation
6. Use `exit` to terminate the application

## 6. Technical Notes

- Uses `std::barrier`, `std::latch`, `std::atomic`, `std::mutex`, and a `std::condition_variable`-based command queue
- `NUM_MARQUEE_HANDLERS` is 4 and exactly 4 threads participate in the barrier/latch so there is no deadlock
- Console output is synchronized using `coutMutex` to prevent interleaved lines
- Keyboard handling is OS-dependent:
  - Windows: `_kbhit/_getch`
  - POSIX: `termios` raw + `select()` + `read()`
