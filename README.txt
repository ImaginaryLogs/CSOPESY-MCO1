Note: This is a .txt version containing some snippets from our README.md.

Made by the following students of CSOPESY S13:

- Christian Joseph C. Bunyi
- Enzo Rafael S. Chan
- Roan Cedric V. Campo
- Mariella Jeanne A. Dellosa

The main function can be found in src/main.cpp.

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
