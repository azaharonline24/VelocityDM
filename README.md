# ⚡ VelocityDM — High-Performance Download Manager

> Open-source IDM clone built with C++20. Multi-threaded downloads, resume support, browser integration.

[![License: MIT](https://img.shields.io/badge/license-MIT-green.svg)](./LICENSE)
[![C++](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![Platform](https://img.shields.io/badge/platform-Windows-blue.svg)](https://www.microsoft.com/windows)
[![Build](https://github.com/azaharonline24/VelocityDM/actions/workflows/build.yml/badge.svg)](https://github.com/azaharonline24/VelocityDM/actions)

---

## ✨ Features

| Feature | Description |
|---------|-------------|
| ⚡ **Multi-threaded** | Split files into segments, download in parallel (up to 8x faster) |
| 🔄 **Resume support** | Pause anytime, resume even after app restart |
| 🌐 **Browser integration** | Chrome extension auto-catches downloads |
| 📊 **Real-time UI** | Progress bars, speed, ETA per segment |
| 🪶 **Lightweight** | Native C++ Win32 — minimal RAM & CPU |
| 🔌 **Protocol support** | HTTP, HTTPS, FTP with range requests |

---

## 🏗️ Architecture

```
┌──────────────────────────────────────────┐
│              VelocityDM                  │
├──────────────────────────────────────────┤
│                                          │
│  Downloader (orchestrator)               │
│  ├─ fetchHeaders() → HEAD request        │
│  ├─ calculateSegments() → split file     │
│  ├─ start() → launch N threads           │
│  ├─ pause()/resume() → save/load state   │
│  └─ getInfo() → progress/speed/status    │
│                                          │
│  SegmentWorker (per-thread)              │
│  ├─ HTTP Range: bytes=start-end          │
│  ├─ writeCallback() → direct disk write  │
│  ├─ 3 retries with exponential backoff   │
│  └─ Thread-safe with std::mutex          │
│                                          │
│  FileIO (disk operations)                │
│  ├─ Pre-allocate file (no fragmentation) │
│  ├─ writeAt(offset) → random access      │
│  └─ Thread-safe writes                   │
│                                          │
│  StateManager (persistence)              │
│  ├─ Save state to .velocity_state JSON   │
│  ├─ Load state for resume                │
│  └─ List/delete states                   │
│                                          │
│  MainWindow (Win32 UI)                   │
│  ├─ ListView with progress bars          │
│  ├─ Toolbar: Add/Pause/Resume/Delete     │
│  ├─ 100ms update timer                   │
│  └─ 0% CPU when idle                     │
│                                          │
│  Browser Extension (Chrome)              │
│  ├─ Manifest V3                          │
│  ├─ Intercept downloads                  │
│  └─ Native Messaging → VelocityDM        │
│                                          │
└──────────────────────────────────────────┘
```

---

## 📋 Prerequisites

Before building, install these tools:

| Tool | Version | Download |
|------|---------|----------|
| **Visual Studio 2022** | Any | [visualstudio.microsoft.com](https://visualstudio.microsoft.com/) — Select "Desktop development with C++" |
| **CMake** | 3.20+ | [cmake.org](https://cmake.org/download/) — Add to PATH |
| **vcpkg** | Latest | See below |
| **Git** | Any | [git-scm.com](https://git-scm.com/) |

---

## 🚀 Build Instructions

### Step 1: Install vcpkg (one-time)

```powershell
# Open PowerShell
cd C:\
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install
```

This gives you a path like `C:\vcpkg` — you'll need it below.

### Step 2: Clone VelocityDM

```powershell
git clone https://github.com/azaharonline24/VelocityDM.git
cd VelocityDM
```

### Step 3: Build

**Option A: Use the build script (easiest)**
```powershell
# Open "Developer Command Prompt for VS 2022"
# (Search in Start Menu)
build.bat
```

**Option B: Manual build**
```powershell
# Install libcurl
vcpkg install curl:x64-windows-static

# Download nlohmann/json
curl -sL "https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp" -o vendor\json.hpp

# Configure (replace C:\vcpkg with your vcpkg path)
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="C:\vcpkg\scripts\buildsystems\vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows-static

# Build
cmake --build build --config Release
```

### Step 4: Find your .exe

```
build\bin\Release\
├── VelocityDM.exe       ← Main application
└── VelocityDMHost.exe   ← Browser native messaging host
```

---

## 🖥️ Usage

### CLI Mode

```powershell
# Basic download
VelocityDM.exe https://example.com/large-file.zip C:\Downloads\

# With custom thread count
VelocityDM.exe --threads 16 https://example.com/large-file.zip C:\Downloads\

# Resume paused download
VelocityDM.exe --resume https://example.com/large-file.zip C:\Downloads\
```

### GUI Mode

```powershell
VelocityDM.exe --gui
```

### Output

```
[Main] Starting download: https://example.com/file.zip
[Main] Threads: 8
[Progress] 45.3% | 12.4 MB/s | file.zip
[Progress] 100.0% | 15.2 MB/s | file.zip
[Main] Download complete!
```

---

## 🌐 Browser Extension Setup

### 1. Load the Extension

1. Open Chrome → `chrome://extensions/`
2. Enable **Developer mode** (top right)
3. Click **Load unpacked**
4. Select the `extension/` folder from the VelocityDM directory

### 2. Register Native Host

Create a registry file (`install.reg`) and run it:

```reg
Windows Registry Editor Version 5.00

[HKEY_CURRENT_USER\Software\Google\Chrome\NativeMessagingHosts\com.velocitydm]
@="C:\\path\\to\\VelocityDM\\src\\browser_bridge\\manifest.json"
```

**Replace** `C:\\path\\to\\VelocityDM` with your actual path.

### 3. Update manifest.json

Edit `src/browser_bridge/manifest.json` and update:
- `path`: Full path to `VelocityDMHost.exe`
- `allowed_origins`: Your extension ID (from chrome://extensions)

### 4. Test

1. Click a download link in Chrome
2. VelocityDM should intercept and download it
3. Check the extension popup for status

---

## 📁 Project Structure

```
VelocityDM/
├── vendor/
│   ├── curl/                    ← libcurl (via vcpkg)
│   └── json.hpp                 ← nlohmann JSON (single header)
├── src/
│   ├── core/
│   │   ├── Downloader.h/cpp     ← Main orchestrator
│   │   ├── SegmentWorker.h/cpp  ← Per-thread download
│   │   ├── FileIO.h/cpp         ← Thread-safe file I/O
│   │   └── StateManager.h/cpp   ← Pause/resume state
│   ├── ui/
│   │   ├── MainWindow.h/cpp     ← Win32 GUI
│   │   └── resource.rc          ← App icon/version
│   ├── browser_bridge/
│   │   ├── NativeHost.cpp       ← Browser messaging host
│   │   └── manifest.json        ← Chrome native host config
│   └── main.cpp                 ← Entry point
├── extension/
│   ├── manifest.json            ← Chrome extension (Manifest V3)
│   ├── background.js            ← Download interceptor
│   ├── popup.html/js            ← Extension popup UI
├── CMakeLists.txt               ← Build configuration
├── build.bat                    ← Windows build script
├── build.sh                     ← Linux cross-compile script
├── README.md
└── LICENSE
```

---

## 🔧 Troubleshooting

### "Could not find toolchain file: vcpkg.cmake"

The `CMAKE_TOOLCHAIN_FILE` path is wrong. Find your vcpkg path:
```powershell
vcpkg integrate install
# It will show the path
```

### "Cannot open include file: curl/curl.h"

libcurl not installed via vcpkg:
```powershell
vcpkg install curl:x64-windows-static
```

### "json.hpp not found"

Download it:
```powershell
curl -sL "https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp" -o vendor\json.hpp
```

### Build succeeds but .exe crashes

Make sure you're using `x64-windows-static` triplet (not just `x64-windows`).

### Extension doesn't catch downloads

1. Check Native Host registry key path
2. Check `manifest.json` has correct `path` to `VelocityDMHost.exe`
3. Check `allowed_origins` matches your extension ID

---

## 📊 Performance

| File Size | Threads | Speed (typical) |
|-----------|---------|-----------------|
| 10 MB | 4 | 2-3x single-thread |
| 100 MB | 8 | 5-8x single-thread |
| 1 GB | 8 | Near line-speed |

Performance depends on server support for Range requests and your bandwidth.

---

## 🤝 Contributing

1. Fork the repo
2. Create a feature branch
3. Make your changes
4. Test on Windows with Visual Studio 2022
5. Submit a PR

### Good first contributions:
- Add download URL dialog to GUI
- Add download history
- Add speed limiter
- Support for more protocols (SFTP, BitTorrent)
- Dark mode UI

---

## 📄 License

MIT License — see [LICENSE](LICENSE).

Free to use, modify, and distribute.
