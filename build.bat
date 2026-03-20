@echo off
REM ═══════════════════════════════════════════════════════
REM VelocityDM — Windows Build Script
REM ═══════════════════════════════════════════════════════
REM Run this in a "Developer Command Prompt for VS 2022"

echo.
echo ╔══════════════════════════════════════════════╗
echo ║        VelocityDM Build Script               ║
echo ╚══════════════════════════════════════════════╝
echo.

REM Check for vcpkg
where vcpkg >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] vcpkg not found. Install from: https://github.com/microsoft/vcpkg
    echo.
    echo Quick install:
    echo   git clone https://github.com/microsoft/vcpkg.git
    echo   cd vcpkg ^&^& bootstrap-vcpkg.bat
    echo   vcpkg integrate install
    exit /b 1
)

REM Install libcurl
echo [1/4] Installing libcurl via vcpkg...
vcpkg install curl:x64-windows-static
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Failed to install libcurl
    exit /b 1
)

REM Download nlohmann/json
echo [2/4] Downloading nlohmann/json...
if not exist vendor mkdir vendor
curl -sL "https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp" -o vendor/json.hpp

REM Configure
echo [3/4] Configuring CMake...
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows-static
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] CMake configure failed
    exit /b 1
)

REM Build
echo [4/4] Building...
cmake --build build --config Release
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Build failed
    exit /b 1
)

echo.
echo ╔══════════════════════════════════════════════╗
echo ║        ✅ Build Complete!                    ║
echo ╠══════════════════════════════════════════════╣
echo ║                                              ║
echo ║  Output: build\bin\Release\                  ║
echo ║    - VelocityDM.exe       (Main app)         ║
echo ║    - VelocityDMHost.exe   (Browser host)     ║
echo ║                                              ║
echo ╚══════════════════════════════════════════════╝
echo.
