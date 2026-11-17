@echo off
cd /d "%~dp0"
set "GAME_EXE=build\sr2-enhanced.exe"
if not exist "%GAME_EXE%" set "GAME_EXE=build\sr2port.exe"
if not exist "%GAME_EXE%" (
  echo Build the game first with build-enhanced.ps1 or CMake.
  pause
  exit /b 1
)
if exist "game_original\" (
  "%GAME_EXE%" --game-dir game_original --fps 60 v adlib %*
) else (
  "%GAME_EXE%" --game-dir Game --fps 60 v adlib %*
)
