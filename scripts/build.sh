#!/usr/bin/env bash
set -euo pipefail

mkdir -p obj bin

# Prefer clang++, fallback to g++
CXX=${CXX:-clang++}
command -v "$CXX" >/dev/null 2>&1 || CXX=${CXX_FALLBACK:-g++}

CXXFLAGS="-std=c++20 -O2 -Wall -Wextra -Wpedantic -pthread -DNOMINMAX"

# Compile each source into obj/*.obj (keeps same extension across OSes)
$CXX $CXXFLAGS -c src/main.cpp                              -o obj/main.obj
$CXX $CXXFLAGS -c src/os_agnostic/CommandHandler.cpp        -o obj/CommandHandler.obj
$CXX $CXXFLAGS -c src/os_agnostic/DisplayHandler.cpp        -o obj/DisplayHandler.obj
$CXX $CXXFLAGS -c src/os_agnostic/KeyboardHandler.cpp       -o obj/KeyboardHandler.obj
$CXX $CXXFLAGS -c src/os_agnostic/MarqueeConsole.cpp        -o obj/MarqueeConsole.obj
$CXX $CXXFLAGS -c src/os_dependent/Scanner_posix.cpp        -o obj/Scanner_posix.obj

# Link
$CXX $CXXFLAGS \
  obj/main.obj obj/CommandHandler.obj obj/DisplayHandler.obj \
  obj/KeyboardHandler.obj obj/MarqueeConsole.obj obj/Scanner_posix.obj \
  -o bin/app

echo
echo "Build succeeded: bin/app"
