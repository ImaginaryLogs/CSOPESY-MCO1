# Marquee Console <!-- omit from toc -->

![title](./assets/readme/title.jpg)

<!-- Refer to https://shields.io/badges for usage -->

![Year, Term, Course](https://img.shields.io/badge/AY2526--T1-CSOPESY-blue) ![C++](https://img.shields.io/badge/C++-%2300599C.svg?logo=c%2B%2B&logoColor=white)

A command-line marquee console application that displays scrolling text with customizable speed and controls. Built with modern C++20/23 features including threading, synchronization primitives, and cross-platform compatibility.

## Table of Contents <!-- omit from toc -->

- [1. Overview](#1-overview)
  - [1.1. Console UI](#11-console-ui)
  - [1.2. Command interpreter](#12-command-interpreter)
  - [1.3. Display handler](#13-display-handler)
  - [1.4. Keyboard handler](#14-keyboard-handler)
  - [1.5. Marquee logic](#15-marquee-logic)
  - [1.6. File handler](#16-file-handler)
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

### 1.1. Console UI

The main console interface that coordinates all components and provides the user interaction layer. It manages the overall application lifecycle and thread coordination.

### 1.2. Command interpreter

Handles command parsing and execution. Processes user commands from a thread-safe queue and coordinates with other components to execute marquee operations.

### 1.3. Display handler

Manages the visual output and marquee animation rendering. Handles console display updates and text scrolling with customizable refresh rates.

### 1.4. Keyboard handler

Provides cross-platform keyboard input handling. Uses OS-specific implementations for Windows (`_kbhit/_getch`) and POSIX systems (`termios` raw + `select()` + `read()`).

### 1.5. Marquee logic

Implements the core scrolling algorithm and animation timing. Manages text rotation and display updates with thread-safe synchronization.

### 1.6. File handler

Optional component for loading text content from ASCII files. Provides the `load_file` command functionality for external text input.

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

**Extra Commands (optional):**

- `load_file <path>` — loads ASCII file into marquee text

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
