#!/bin/bash
# ═══════════════════════════════════════════════════════
# VelocityDM — Cross-compile for Windows (Linux host)
# ═══════════════════════════════════════════════════════
# Requires: mingw-w64

set -e

echo "🔧 VelocityDM Cross-Compile (Linux → Windows)"
echo "=============================================="

# Check for MinGW
if ! command -v x86_64-w64-mingw32-g++ &> /dev/null; then
    echo "📦 Installing MinGW-w64..."
    apt-get update -qq && apt-get install -y -qq mingw-w64
fi

# Download nlohmann/json if missing
if [ ! -f vendor/json.hpp ]; then
    echo "📥 Downloading nlohmann/json..."
    curl -sL "https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp" -o vendor/json.hpp
fi

# Download prebuilt libcurl for Windows
echo "📥 Downloading libcurl for Windows..."
VENDOR_DIR="vendor/curl"
mkdir -p "$VENDOR_DIR"

# Use a minimal approach: compile with static libcurl
# For full build, use the GitHub Actions workflow on Windows

echo ""
echo "⚠️  Cross-compilation has limitations."
echo "For a full Windows build with all features, use:"
echo "  1. GitHub Actions (automatic on tag push)"
echo "  2. Or run build.bat on Windows with Visual Studio 2022"
echo ""
echo "Attempting minimal build..."

# Compile main app (simplified, without full libcurl linking)
x86_64-w64-mingw32-g++ -std=c++20 -O2 \
    -Ivendor -Isrc \
    -DWIN32 -D_WIN32_WINNT=0x0A00 \
    src/main.cpp \
    src/core/FileIO.cpp \
    src/core/StateManager.cpp \
    -o VelocityDM.exe \
    -lws2_32 -lshlwapi -lstdc++fs 2>/dev/null || echo "⚠️  Full build needs Windows + Visual Studio"

echo ""
echo "For proper Windows .exe with libcurl, push a tag to trigger GitHub Actions:"
echo "  git tag v1.0.0"
echo "  git push origin v1.0.0"
