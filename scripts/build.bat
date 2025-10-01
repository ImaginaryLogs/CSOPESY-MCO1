@echo off
setlocal

REM Ensure output folders
if not exist obj mkdir obj
if not exist bin mkdir bin

REM C++20, warnings, PDB, parallel build; avoid Windows min/max macros
cl ^
  /nologo /std:c++20 /EHsc /W4 /Zi /MP /utf-8 /DNOMINMAX ^
  /Foobj\ /Fd:bin\app.pdb /Fe:bin\app.exe ^
  src\main.cpp ^
  src\os_agnostic\CommandHandler.cpp ^
  src\os_agnostic\DisplayHandler.cpp ^
  src\os_agnostic\KeyboardHandler.cpp ^
  src\os_agnostic\MarqueeConsole.cpp ^
  src\os_dependent\Scanner_win32.cpp

if errorlevel 1 (
  echo.
  echo Build failed.
  exit /b 1
)

echo.
echo Build succeeded: bin\app.exe
exit /b 0
