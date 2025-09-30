# Marquee CLI (C++20/23)

## Build
### Linux/macOS/WSL
```bash
make
./bin/app
```

### Windows (VS 2022 Developer Command Prompt)
```bat
cl /std:c++20 /EHsc src\main.cpp src\os_agnostic\*.cpp src\os_dependent\*.cpp /Fe:bin\app.exe
```

## Commands (per PDF spec)
- `help` — displays the commands and its description
- `start_marquee` — starts the marquee animation
- `stop_marquee` — stops the marquee animation
- `set_text <text>` — sets marquee text
- `set_speed <ms>` — sets refresh in milliseconds
- `exit` — terminates the console

Extra (not part of spec, optional):
- `load_file <path>` — loads ASCII file into marquee text

## Notes
- Uses `std::barrier`, `std::latch`, `std::atomic`, `std::mutex`, and a `std::condition_variable`-based command queue.
- `NUM_MARQUEE_HANDLERS` is 4 and exactly 4 threads participate in the barrier/latch so there is no deadlock.
- Console output is synchronized using `coutMutex` to prevent interleaved lines.
- Keyboard handling is OS-dependent:
  - Windows: `_kbhit/_getch`
  - POSIX: `termios` raw + `select()` + `read()`
