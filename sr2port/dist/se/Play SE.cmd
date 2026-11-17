@echo off
setlocal
cd /d "%~dp0"
if not exist "Game\SR.EXE" if not exist "Game\SR2.EXE" (
  echo Copy your original Street Rod 2 files into the Game folder first.
  echo See README.txt for Czech and English instructions.
  pause
  exit /b 1
)
"%~dp0Street Rod 2 SE.exe" --game-dir Game --fps 60 v adlib %*
if errorlevel 1 (
  echo The game could not start. Check the files in Game; see README.txt.
  pause
  exit /b 1
)
