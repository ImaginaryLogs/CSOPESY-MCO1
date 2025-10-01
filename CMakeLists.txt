cmake_minimum_required(VERSION 3.20)

project(MarqueeConsole LANGUAGES CXX)

# C++20 across platforms
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output folders: put executables in bin/, libraries in bin/, archives in obj/
# (Object files themselves live under the build dir; see CMakePresets.json to use build dir = obj/)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/obj)

# For multi-config generators (VS, Ninja Multi-Config), ensure per-config dirs don’t append subfolders
foreach(OUT RUNTIME LIBRARY ARCHIVE)
  foreach(CONFIG Debug Release RelWithDebInfo MinSizeRel)
    set(CMAKE_${OUT}_OUTPUT_DIRECTORY_${CONFIG} ${CMAKE_${OUT}_OUTPUT_DIRECTORY})
  endforeach()
endforeach()

# Sources (no FileReader* since you removed that feature)
set(SRC_COMMON
  src/main.cpp
  src/os_agnostic/CommandHandler.cpp
  src/os_agnostic/DisplayHandler.cpp
  src/os_agnostic/KeyboardHandler.cpp
  src/os_agnostic/MarqueeConsole.cpp
)

if (WIN32)
  list(APPEND SRC_COMMON src/os_dependent/Scanner_win32.cpp)
else()
  list(APPEND SRC_COMMON src/os_dependent/Scanner_posix.cpp)
endif()

add_executable(app ${SRC_COMMON})

# Compiler options
if (MSVC)
  target_compile_options(app PRIVATE /W4 /EHsc /permissive- /utf-8 /Zc:preprocessor)
  target_compile_definitions(app PRIVATE NOMINMAX)
  # Put PDBs in bin/
  set_target_properties(app PROPERTIES
    PDB_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
  )
else()
  target_compile_options(app PRIVATE -Wall -Wextra -Wpedantic -O2)
endif()

# Threads (pthread on POSIX; noop on MSVC)
find_package(Threads REQUIRED)
target_link_libraries(app PRIVATE Threads::Threads)
