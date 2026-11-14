@echo off
setlocal
cd /d "%~dp0"
echo.
echo SmartPark TRACE - Building C++ web engine
call compile_bridge.bat -nopause
if errorlevel 1 (
  echo C++ compiler not detected. See README.
  pause
  exit /b 1
)
if exist ".venv\Scripts\python.exe" (
    ".venv\Scripts\python.exe" server\app.py
) else (
    echo No .venv detected. Run setup_windows.bat first.
    pause
    exit /b 1
)
pause
