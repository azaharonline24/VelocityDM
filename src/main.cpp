// ═══════════════════════════════════════════════════════
// VelocityDM — Application Entry Point
// ═══════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <filesystem>
#include "core/Downloader.h"
#include "core/StateManager.h"
#include "ui/MainWindow.h"

namespace fs = std::filesystem;

void printUsage() {
    std::cout << "VelocityDM - High-Performance Download Manager\n"
              << "Usage:\n"
              << "  VelocityDM.exe <url> <output_path> [--threads N]\n"
              << "  VelocityDM.exe --gui           Launch GUI mode\n"
              << "\nOptions:\n"
              << "  --threads N   Number of download threads (default: 8)\n"
              << "  --gui         Launch graphical interface\n"
              << "  --resume      Resume paused downloads in output path\n"
              << "\nExamples:\n"
              << "  VelocityDM.exe https://example.com/file.zip C:\\Downloads\\\n"
              << "  VelocityDM.exe --threads 16 https://example.com/file.zip C:\\Downloads\\\n"
              << std::endl;
}

int runCLI(int argc, char* argv[]) {
    std::string url;
    std::string outputPath;
    int threads = 8;
    bool resumeMode = false;

    // Parse arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--threads" && i + 1 < argc) {
            threads = std::stoi(argv[++i]);
        } else if (arg == "--resume") {
            resumeMode = true;
        } else if (arg.rfind("http", 0) == 0) {
            url = arg;
        } else if (fs::exists(arg) || arg.find(':') != std::string::npos) {
            outputPath = arg;
        }
    }

    if (url.empty()) {
        printUsage();
        return 1;
    }

    // Ensure output directory exists
    if (!outputPath.empty()) {
        FileIO::ensureDirectory(outputPath);
    }

    // Create downloader
    VelocityDM::Downloader downloader;

    // Check for resume state
    if (resumeMode) {
        std::string filename = FileIO::extractFilename(url, outputPath);
        if (VelocityDM::StateManager::hasState(outputPath, filename)) {
            std::cout << "[Main] Resuming download: " << filename << std::endl;
            // TODO: Load state and resume
        }
    }

    // Start download
    std::cout << "[Main] Starting download: " << url << std::endl;
    std::cout << "[Main] Threads: " << threads << std::endl;

    if (!downloader.start(url, outputPath, threads)) {
        std::cerr << "[Main] Failed to start download" << std::endl;
        return 1;
    }

    // Progress loop (CLI mode)
    while (downloader.isActive()) {
        auto info = downloader.getInfo();

        // Format speed
        std::string speedStr;
        if (info.speed > 1024 * 1024) {
            speedStr = std::to_string(static_cast<int>(info.speed / 1024 / 1024)) + " MB/s";
        } else if (info.speed > 1024) {
            speedStr = std::to_string(static_cast<int>(info.speed / 1024)) + " KB/s";
        } else {
            speedStr = std::to_string(static_cast<int>(info.speed)) + " B/s";
        }

        // Print progress
        std::cout << "\r[Progress] " << std::fixed << std::setprecision(1)
                  << info.progress << "% | " << speedStr
                  << " | " << info.filename << "     " << std::flush;

        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Check if all segments complete
        bool allDone = true;
        for (const auto& seg : downloader.getSegments()) {
            if (seg.status.load() != VelocityDM::SegmentStatus::COMPLETED) {
                allDone = false;
                break;
            }
        }
        if (allDone) break;
    }

    std::cout << "\n[Main] Download complete!" << std::endl;

    // Clean up state file
    std::string filename = FileIO::extractFilename(url, outputPath);
    VelocityDM::StateManager::deleteState(outputPath, filename);

    return 0;
}

int main(int argc, char* argv[]) {
    // Check for GUI mode
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--gui") {
            return VelocityDM::UI::runMainWindow();
        }
    }

    // If no arguments or --gui not found, try CLI mode
    if (argc < 2) {
        // Check for saved states to resume
        auto states = VelocityDM::StateManager::listStates(".");
        if (!states.empty()) {
            std::cout << "[Main] Found " << states.size()
                      << " paused download(s). Use --resume to continue." << std::endl;
        }
        printUsage();
        return 0;
    }

    return runCLI(argc, argv);
}
