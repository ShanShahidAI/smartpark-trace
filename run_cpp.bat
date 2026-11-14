@echo off
setlocal
cd /d "%~dp0"
if not exist "build" mkdir "build"
call "FullStack_Web\detect_cpp_compiler.bat"
if errorlevel 1 (
    echo No C++17 compiler found. Install g++ or Visual Studio C++ Build Tools.
    pause
    exit /b 1
)
where g++ >nul 2>nul
if not errorlevel 1 (
    g++ -std=c++17 -O2 -Wall "Standalone_CPP\SmartPark.cpp" -o "build\smartpark_cpp.exe"
    goto ready
)
where cl >nul 2>nul
if not errorlevel 1 (
    cl /nologo /std:c++17 /EHsc /O2 "Standalone_CPP\SmartPark.cpp" /Fe"build\smartpark_cpp.exe"
    goto ready
)
echo No C++17 compiler found. Run in x64 Native Tools Command Prompt or install g++.
pause
exit /b 1
:ready
if errorlevel 1 (pause & exit /b 1)
set "SMARTPARK_RATE_FILE=%~dp0Standalone_CPP\data\parking_rates.txt"
"build\smartpark_cpp.exe" --data "Standalone_CPP\data\cpp_state.txt"
pause
