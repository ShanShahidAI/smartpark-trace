@echo off
rem Find an installed C++17 compiler. This file is called by the other launchers.
rem It deliberately does not use SETLOCAL, so the MSVC environment reaches the caller.
where g++ >nul 2>nul
if not errorlevel 1 exit /b 0
where cl >nul 2>nul
if not errorlevel 1 exit /b 0
rem When Visual Studio is installed, initialize its x64 compiler automatically.
for %%Y in (2026 2022 2019) do (
  for %%E in (Community Professional Enterprise BuildTools) do (
    if exist "%ProgramFiles%\Microsoft Visual Studio\%%Y\%%E\VC\Auxiliary\Build\vcvars64.bat" (
      call "%ProgramFiles%\Microsoft Visual Studio\%%Y\%%E\VC\Auxiliary\Build\vcvars64.bat" >nul
      where cl >nul 2>nul
      if not errorlevel 1 exit /b 0
    )
  )
)
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" (
  call "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
  where cl >nul 2>nul
  if not errorlevel 1 exit /b 0
)
exit /b 1
