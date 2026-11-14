@echo off
setlocal
cd /d "%~dp0"
if not exist "build" mkdir "build"
echo Building SmartPark C++ API engine...
set "SOURCES=backend\web_bridge.cpp backend\Vehicle.cpp backend\ParkingSlot.cpp backend\ParkingLot.cpp backend\LinkedList.cpp backend\VehicleQueue.cpp backend\VehicleStack.cpp backend\PriorityQueue.cpp backend\Search.cpp backend\Sort.cpp backend\FeeCalculator.cpp backend\DoublyLinkedList.cpp backend\CircularLinkedList.cpp backend\DynamicVehicleArray.cpp backend\ParkingSystem.cpp backend\BST.cpp backend\AVLTree.cpp"
call "%~dp0detect_cpp_compiler.bat"
if errorlevel 1 goto no_compiler
where g++ >nul 2>nul
if not errorlevel 1 (
    g++ -std=c++17 -O2 -Ibackend %SOURCES% -o "build\smartpark_bridge.exe"
    goto result
)
where cl >nul 2>nul
if not errorlevel 1 (
    cl /nologo /std:c++17 /EHsc /O2 /Ibackend /Fo"build\\" %SOURCES% /Fe"build\smartpark_bridge.exe"
    goto result
)
goto no_compiler
:no_compiler
echo ERROR: No C++17 compiler found on PATH.
echo Install MinGW-w64 g++ or open a Visual Studio x64 Native Tools Command Prompt.
if not "%~1"=="-nopause" pause
exit /b 1
:result
if errorlevel 1 (
    echo ERROR: C++ bridge compilation failed.
    if not "%~1"=="-nopause" pause
    exit /b 1
)
echo SUCCESS: build\smartpark_bridge.exe
if not "%~1"=="-nopause" pause
exit /b 0
