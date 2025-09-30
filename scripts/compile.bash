#!/usr/bin/env bash
set -e
mkdir -p bin
c++ -std=c++23 -Wall -Wextra -O2 \
  src/main.cpp \
  src/os_agnostic/DisplayHandler.cpp \
  src/os_agnostic/KeyboardHandler.cpp \
  src/os_agnostic/CommandHandler.cpp \
  src/os_agnostic/FileReaderHandler.cpp \
  src/os_agnostic/MarqueeConsole.cpp \
  src/os_dependent/Scanner_win32.cpp \
  src/os_dependent/Scanner_posix.cpp \
  -pthread \
  -o bin/app
echo "Built bin/app"
