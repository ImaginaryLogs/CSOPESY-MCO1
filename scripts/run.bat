@echo off
if not exist bin\app.exe (
  echo bin\app.exe not found. Run scripts\build.bat first.
  exit /b 1
)
bin\app.exe
