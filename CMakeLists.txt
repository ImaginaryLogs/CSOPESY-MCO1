cmake_minimum_required(VERSION 3.16)
project(MarqueeConsole LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_executable(marquee
    src/main.cpp
    src/Context.cpp
    src/os_agnostic/Marquee.cpp
    src/os_agnostic/CommandHandler.cpp
    src/os_agnostic/DisplayHandler.cpp
    src/os_agnostic/KeyboardHandler.cpp
    src/os_dependent/ScannerLibrary.cpp
)
