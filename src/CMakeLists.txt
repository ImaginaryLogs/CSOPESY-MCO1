cmake_minimum_required(VERSION 3.10)
project(MarqueeConsole)

set(CMAKE_CXX_STANDARD 17)

# Add all source files
add_executable(marquee
    main.cpp
    src/Context.cpp
    src/os_agnostic/Marquee.cpp
    src/os_agnostic/DisplayHandler.cpp
    src/os_agnostic/CommandHandler.cpp
    src/os_agnostic/KeyboardHandler.cpp
    src/os_dependent/ScannerLibrary.cpp
)
