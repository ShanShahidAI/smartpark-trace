@echo off
setlocal
cd /d "%~dp0"
where py >nul 2>nul
if %errorlevel%==0 (
    set "PYTHON=py -3"
) else (
    set "PYTHON=python"
)
echo.
echo Installing SmartPark requirements in .venv ...
%PYTHON% -m venv .venv
if errorlevel 1 (
    echo Python 3 is required. Install from https://www.python.org/downloads/
    pause
    exit /b 1
)
".venv\Scripts\python.exe" -m pip install -r requirements.txt
if errorlevel 1 (
    echo Flask installation failed. Check your Internet connection and retry.
    pause
    exit /b 1
)
echo.
echo Setup complete. Double-click run_smartpark.bat to start.
pause
