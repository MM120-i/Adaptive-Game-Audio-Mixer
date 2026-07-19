@echo off
setlocal enabledelayedexpansion

:: Find cmake
set CMAKE=
if exist "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
    set CMAKE="C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
) else (
    where cmake >nul 2>&1
    if %ERRORLEVEL% equ 0 set CMAKE=cmake
)

if not defined CMAKE (
    echo Cannot find cmake.exe. Open "Developer PowerShell for VS" or install cmake.
    exit /b 1
)

if /i "%1"=="help" (
    echo --------------------------------------------------
    echo   AudioMixer  -  available targets
    echo --------------------------------------------------
    echo   .\make.bat             build app + tests ^(Debug^)
    echo   .\make.bat Debug       build specific config
    echo   .\make.bat run         launch the app
    echo   .\make.bat run Debug   launch Release config
    echo   .\make.bat test        build + run tests
    echo   .\make.bat clean       delete build/ directory
    echo   .\make.bat help        show this message
    echo --------------------------------------------------
    echo   ^>^> also available:
    echo   .\test.ps1             build + verbose tests
    echo   make                   ^(Makefile, same targets^)
    echo --------------------------------------------------
    exit /b 0
)

if /i "%1"=="clean" (
    echo ==> Cleaning build directory...
    if exist build\ rmdir /s /q build
    echo Done.
    exit /b 0
)

if /i "%1"=="run" (
    set RUNCONFIG=Debug
    if not "%2"=="" set RUNCONFIG=%2
    if not exist "build\AudioMixer_artefacts\!RUNCONFIG!\AudioMixer.exe" (
        echo App not built. Run: .\make.bat !RUNCONFIG! first.
        exit /b 1
    )
    echo ==> Launching AudioMixer ^(!RUNCONFIG!^)...
    start "" "build\AudioMixer_artefacts\!RUNCONFIG!\AudioMixer.exe"
    exit /b 0
)

if /i "%1"=="test" (
    if "%2"=="" (set CONFIG=Debug) else (set CONFIG=%2)
    echo ==> Building + testing ^(!CONFIG!^)...
    %CMAKE% --build build --config !CONFIG! --target AudioMixerTests --parallel
    if !ERRORLEVEL! neq 0 exit /b !ERRORLEVEL!
    echo.
    powershell -ExecutionPolicy Bypass -File test.ps1 -Config !CONFIG!
    exit /b 0
)

if "%1"=="" (
    set CONFIG=Debug
) else (
    set CONFIG=%1
)

echo ==> Building AudioMixer (%CONFIG%)...
if not exist build\CMakeCache.txt (
    echo ==> Configuring CMake...
    %CMAKE% -S . -B build -G "Visual Studio 18 2026" -A x64
    if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%
)
%CMAKE% --build build --config %CONFIG% --parallel
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%

echo.
echo ==> Building tests...
%CMAKE% --build build --config %CONFIG% --target AudioMixerTests --parallel
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%

if "%2"=="test" (
    echo.
    powershell -ExecutionPolicy Bypass -File test.ps1 -Config %CONFIG%
)
