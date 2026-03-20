# Velocity Download Manager (IDM Clone)

> 🚀 High-performance download manager built with C++20. Multi-threaded, resume support, browser integration.

[![License: MIT](https://img.shields.io/badge/license-MIT-green.svg)](./LICENSE)
[![C++](https://img.shields.io/badge/c%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![CMake](https://img.shields.io/badge/cmake-3.20+-blue.svg)](https://cmake.org)
[![Platform](https://img.shields.io/badge/platform-Windows-blue.svg)](https://www.microsoft.com/windows)

## ✨ Features

- ⚡ **Multi-threaded downloading** — Split files into segments for maximum speed
- 🔄 **Resume support** — Pause and resume downloads anytime, even after restart
- 🌐 **Browser integration** — Auto-catch downloads from Chrome, Firefox, Edge
- 📊 **Real-time progress** — Speed, ETA, progress bars per segment
- 🪶 **Lightweight** — Native C++ Win32, minimal RAM & CPU usage
- 🔌 **Protocol support** — HTTP, HTTPS, FTP with range requests

## 🏗️ Architecture

```
/VelocityDM
├── /vendor
│   ├── /curl              (libcurl static libraries)
│   └── json.hpp           (nlohmann JSON single header)
├── /src
│   ├── /core
│   │   ├── Downloader.h/cpp       (Main download orchestrator)
│   │   ├── SegmentWorker.h/cpp    (Per-thread download worker)
│   │   ├── FileIO.h/cpp           (Thread-safe file operations)
│   │   └── StateManager.h/cpp     (Pause/resume state persistence)
│   ├── /ui
│   │   ├── MainWindow.h/cpp       (Win32 list view + progress bars)
│   │   └── resource.rc            (UI resources)
│   ├── /browser_bridge
│   │   ├── NativeHost.cpp         (Browser native messaging host)
│   │   └── manifest.json
│   └── main.cpp                   (Entry point)
├── /extension
│   ├── manifest.json              (Chrome Manifest V3)
│   ├── background.js              (Download interceptor)
│   └── popup.html
├── CMakeLists.txt
└── README.md
```

## 🚀 Build Instructions

### Prerequisites

- **Visual Studio 2022** (or Build Tools)
- **CMake** 3.20+
- **vcpkg** (for libcurl)

### Quick Build

```bash
# Clone
git clone https://github.com/azaharonline24/VelocityDM.git
cd VelocityDM

# Install dependencies via vcpkg
vcpkg install curl:x64-windows

# Build
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=[vcpkg-root]/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release

# Output
build/Release/VelocityDM.exe
```

### Manual Build (without vcpkg)

1. Download [libcurl](https://curl.se/download.html) precompiled binaries
2. Place in `vendor/curl/`
3. Build with CMake (no toolchain needed)

## 🌐 Browser Extension Setup

1. Open Chrome → `chrome://extensions/`
2. Enable "Developer mode"
3. Click "Load unpacked" → Select `extension/` folder
4. Run the Native Host installer (or manually add registry key)

### Registry Key (Chrome)

```
Windows Registry Editor Version 5.00

[HKEY_CURRENT_USER\Software\Google\Chrome\NativeMessagingHosts\com.velocitydm]
@="C:\\path\\to\\VelocityDM\\src\\browser_bridge\\manifest.json"
```

## 📊 How It Works

```
1. User clicks download in browser
2. Extension intercepts → sends URL to Native Host
3. Native Host → sends URL to VelocityDM via Named Pipe
4. VelocityDM:
   a. HEAD request → get file size + check Range support
   b. Pre-allocate file on disk
   c. Split into N segments (default: 8)
   d. Launch N threads, each downloads a byte range
   e. Write directly to disk (low RAM)
   f. Save state for resume support
5. UI updates every 100ms via timer
```

## 🎯 CLI Usage

```bash
# Basic download
VelocityDM.exe https://example.com/file.zip C:\Downloads\file.zip

# With thread count
VelocityDM.exe --threads 16 https://example.com/file.zip C:\Downloads\file.zip
```

## 📋 Roadmap

- [x] Phase 1: Foundation & Network Core
- [x] Phase 2: Multi-threading & Segmentation
- [x] Phase 3: State Management (Resume)
- [x] Phase 4: Browser Integration
- [x] Phase 5: User Interface

## 📄 License

MIT License — see [LICENSE](LICENSE).
