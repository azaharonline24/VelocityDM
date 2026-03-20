#pragma once
// ═══════════════════════════════════════════════════════
// Downloader — Main download orchestrator
// ═══════════════════════════════════════════════════════

#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <memory>
#include <chrono>
#include "SegmentWorker.h"
#include "FileIO.h"

namespace VelocityDM {

enum class DownloadStatus : int {
    IDLE = 0,
    FETCHING_HEADERS = 1,
    DOWNLOADING = 2,
    PAUSED = 3,
    COMPLETED = 4,
    ERROR = 5
};

struct DownloadInfo {
    std::string url;
    std::string outputPath;
    std::string filename;
    uint64_t totalSize;
    bool supportsRange;
    DownloadStatus status;
    double speed;
    double progress;
    std::string errorMessage;
};

class Downloader {
public:
    Downloader();
    ~Downloader();

    bool fetchHeaders(const std::string& url);
    bool start(const std::string& url, const std::string& outputPath,
               int threadCount = 8);
    void pause();
    void resume();
    void cancel();

    DownloadInfo getInfo() const;
    const std::vector<SegmentWorker>& getSegments() const;
    bool isActive() const;
    double getSpeed() const;

private:
    std::string url_;
    std::string outputPath_;
    std::string filename_;
    uint64_t totalSize_;
    bool supportsRange_;
    int threadCount_;

    std::atomic<int> status_;  // Stores DownloadStatus as int
    std::vector<SegmentWorker> segments_;
    std::vector<std::thread> threads_;
    std::unique_ptr<FileIO> fileIO_;

    std::atomic<bool> running_;
    std::chrono::steady_clock::time_point startTime_;
    std::atomic<uint64_t> lastBytesDownloaded_;
    std::chrono::steady_clock::time_point lastSpeedCheck_;

    DownloadStatus getStatus() const {
        return static_cast<DownloadStatus>(status_.load());
    }

    void setStatus(DownloadStatus s) {
        status_.store(static_cast<int>(s));
    }

    void calculateSegments(int threadCount);
    void joinThreads();
};

void downloadSegment(const std::string& url,
                     SegmentWorker* worker,
                     FileIO* fileIO,
                     const std::string& outputPath);

} // namespace VelocityDM
