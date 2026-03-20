#pragma once
// ═══════════════════════════════════════════════════════
// Downloader — Main download orchestrator
// ═══════════════════════════════════════════════════════

#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <memory>
#include <functional>
#include "SegmentWorker.h"
#include "FileIO.h"

namespace VelocityDM {

enum class DownloadStatus {
    IDLE,
    FETCHING_HEADERS,
    DOWNLOADING,
    PAUSED,
    COMPLETED,
    ERROR
};

struct DownloadInfo {
    std::string url;
    std::string outputPath;
    std::string filename;
    uint64_t totalSize;
    bool supportsRange;
    DownloadStatus status;
    double speed;           // bytes per second
    double progress;        // 0-100
    std::string errorMessage;
};

class Downloader {
public:
    Downloader();
    ~Downloader();

    // Fetch file headers (HEAD request)
    // Returns false if download not possible
    bool fetchHeaders(const std::string& url);

    // Start download with specified thread count
    bool start(const std::string& url, const std::string& outputPath,
               int threadCount = 8);

    // Pause download (saves state)
    void pause();

    // Resume paused download
    void resume();

    // Cancel download
    void cancel();

    // Get current download info
    DownloadInfo getInfo() const;

    // Get segment workers for progress display
    const std::vector<SegmentWorker>& getSegments() const;

    // Check if download is active
    bool isActive() const;

    // Get overall speed (bytes/sec)
    double getSpeed() const;

private:
    std::string url_;
    std::string outputPath_;
    std::string filename_;
    uint64_t totalSize_;
    bool supportsRange_;
    int threadCount_;

    std::atomic<DownloadStatus> status_;
    std::vector<SegmentWorker> segments_;
    std::vector<std::thread> threads_;
    std::unique_ptr<FileIO> fileIO_;

    std::atomic<bool> running_;
    std::chrono::steady_clock::time_point startTime_;
    std::atomic<uint64_t> lastBytesDownloaded_;
    std::chrono::steady_clock::time_point lastSpeedCheck_;

    // Calculate optimal segment boundaries
    void calculateSegments(int threadCount);

    // Download single-threaded (fallback)
    void downloadSingleThread();

    // Cleanup threads
    void joinThreads();
};

// Global download segment function (defined in SegmentWorker.cpp)
void downloadSegment(const std::string& url,
                     SegmentWorker* worker,
                     FileIO* fileIO,
                     const std::string& outputPath);

} // namespace VelocityDM
