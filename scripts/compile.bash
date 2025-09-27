# Use .exe on Windows, otherwise just 'app'
if [[ "$OS" == "Windows_NT" ]] || [[ "$(uname -s)" =~ MINGW|MSYS|CYGWIN ]]; then
    OUTFILE="bin/app.exe"
else
    OUTFILE="bin/app"
fi

g++ -std=c++23 -Wall \
    src/main.cpp \
    src/os_agnostic/Context.cpp \
    src/os_agnostic/DisplayHandler.cpp \
    src/os_agnostic/KeyboardHandler.cpp \
    src/os_agnostic/CommandHandler.cpp \
    src/os_agnostic/FileReaderHandler.cpp \
    src/os_dependent/ScannerLibrary.cpp \
    -o "$OUTFILE"
