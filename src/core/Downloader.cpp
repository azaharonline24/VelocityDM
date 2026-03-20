// ═══════════════════════════════════════════════════════
// Downloader Implementation — Main orchestrator
// ═══════════════════════════════════════════════════════

#include "Downloader.h"
#include <curl/curl.h>
#include <iostream>
#include <algorithm>

namespace VelocityDM {

// Default thread count
static constexpr int DEFAULT_THREADS = 8;
// Minimum file size for multi-threading (1MB)
static constexpr uint64_t MIN_MULTITHREAD_SIZE = 1024 * 1024;

Downloader::Downloader()
    : totalSize_(0), supportsRange_(false), threadCount_(DEFAULT_THREADS),
      status_(DownloadStatus::IDLE), running_(false),
      lastBytesDownloaded_(0) {
    fileIO_ = std::make_unique<FileIO>();
    curl_global_init(CURL_GLOBAL_ALL);
}

Downloader::~Downloader() {
    cancel();
    curl_global_cleanup();
}

bool Downloader::fetchHeaders(const std::string& url) {
    status_.store(DownloadStatus::FETCHING_HEADERS);
    url_ = url;

    CURL* curl = curl_easy_init();
    if (!curl) {
        status_.store(DownloadStatus::ERROR);
        return false;
    }

    // Set HEAD request
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "VelocityDM/1.0");

    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        std::cerr << "[Downloader] HEAD request failed: "
                  << curl_easy_strerror(res) << std::endl;
        curl_easy_cleanup(curl);
        status_.store(DownloadStatus::ERROR);
        return false;
    }

    // Get content length
    curl_off_t contentLength = 0;
    curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &contentLength);
    totalSize_ = static_cast<uint64_t>(contentLength);

    // Check Accept-Ranges support
    char* acceptRanges = nullptr;
    curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &acceptRanges);

    // Try to get Accept-Ranges header
    supportsRange_ = false;
    if (totalSize_ > 0) {
        // We'll check Range support by attempting a range request later
        // For now, assume support if we got a content length
        supportsRange_ = (totalSize_ >= MIN_MULTITHREAD_SIZE);
    }

    // Extract filename from Content-Disposition or URL
    filename_ = FileIO::extractFilename(url, "");

    curl_easy_cleanup(curl);
    status_.store(DownloadStatus::IDLE);
    return true;
}

bool Downloader::start(const std::string& url, const std::string& outputPath,
                       int threadCount) {
    if (isActive()) {
        std::cerr << "[Downloader] Already active" << std::endl;
        return false;
    }

    url_ = url;
    outputPath_ = outputPath;
    threadCount_ = threadCount;

    // Fetch headers first
    if (!fetchHeaders(url)) {
        return false;
    }

    // Determine output file path
    std::string filePath = FileIO::extractFilename(url, outputPath);
    filename_ = fs::path(filePath).filename().string();

    // Check disk space
    uint64_t freeSpace = FileIO::getFreeSpace(outputPath);
    if (totalSize_ > 0 && freeSpace > 0 && totalSize_ > freeSpace) {
        std::cerr << "[Downloader] Not enough disk space. Need "
                  << totalSize_ << ", have " << freeSpace << std::endl;
        status_.store(DownloadStatus::ERROR);
        return false;
    }

    // Pre-allocate file
    if (totalSize_ > 0) {
        if (!FileIO::createEmptyFile(filePath, totalSize_)) {
            status_.store(DownloadStatus::ERROR);
            return false;
        }
    }

    status_.store(DownloadStatus::DOWNLOADING);
    running_.store(true);
    startTime_ = std::chrono::steady_clock::now();
    lastSpeedCheck_ = startTime_;

    // Decide: multi-threaded or single-threaded
    if (supportsRange_ && totalSize_ >= MIN_MULTITHREAD_SIZE && threadCount_ > 1) {
        // Multi-threaded download
        calculateSegments(threadCount_);

        for (auto& segment : segments_) {
            threads_.emplace_back(downloadSegment, url_, &segment,
                                  fileIO_.get(), filePath);
        }
    } else {
        // Single-threaded fallback
        segments_.clear();
        segments_.emplace_back(0, 0, totalSize_ > 0 ? totalSize_ - 1 : 0);
        segments_[0].status.store(SegmentStatus::DOWNLOADING);

        threads_.emplace_back(downloadSegment, url_, &segments_[0],
                              fileIO_.get(), filePath);
    }

    return true;
}

void Downloader::pause() {
    if (!isActive()) return;

    status_.store(DownloadStatus::PAUSED);
    running_.store(false);

    // Signal all segments to pause
    for (auto& segment : segments_) {
        segment.status.store(SegmentStatus::PAUSED);
    }

    // Wait for threads to finish
    joinThreads();

    // Save state for resume
    // (StateManager integration in Phase 3)
}

void Downloader::resume() {
    if (status_.load() != DownloadStatus::PAUSED) return;

    status_.store(DownloadStatus::DOWNLOADING);
    running_.store(true);

    std::string filePath = FileIO::extractFilename(url_, outputPath_);

    // Restart segments that weren't complete
    for (auto& segment : segments_) {
        if (segment.status.load() != SegmentStatus::COMPLETED) {
            segment.status.store(SegmentStatus::DOWNLOADING);
            threads_.emplace_back(downloadSegment, url_, &segment,
                                  fileIO_.get(), filePath);
        }
    }
}

void Downloader::cancel() {
    running_.store(false);
    status_.store(DownloadStatus::IDLE);

    for (auto& segment : segments_) {
        segment.status.store(SegmentStatus::PAUSED);
    }

    joinThreads();
    segments_.clear();
}

DownloadInfo Downloader::getInfo() const {
    DownloadInfo info;
    info.url = url_;
    info.outputPath = outputPath_;
    info.filename = filename_;
    info.totalSize = totalSize_;
    info.supportsRange = supportsRange_;
    info.status = status_.load();
    info.speed = getSpeed();
    info.progress = 0;

    // Calculate overall progress
    if (totalSize_ > 0) {
        uint64_t totalDownloaded = 0;
        for (const auto& segment : segments_) {
            totalDownloaded += segment.getBytesDownloaded();
        }
        info.progress = (static_cast<double>(totalDownloaded) / totalSize_) * 100.0;
    }

    // Check for errors
    for (const auto& segment : segments_) {
        if (segment.status.load() == SegmentStatus::ERROR) {
            info.status = DownloadStatus::ERROR;
            info.errorMessage = segment.errorMessage;
            break;
        }
    }

    // Check if all complete
    bool allComplete = true;
    for (const auto& segment : segments_) {
        if (segment.status.load() != SegmentStatus::COMPLETED) {
            allComplete = false;
            break;
        }
    }
    if (allComplete && !segments_.empty()) {
        info.status = DownloadStatus::COMPLETED;
    }

    return info;
}

const std::vector<SegmentWorker>& Downloader::getSegments() const {
    return segments_;
}

bool Downloader::isActive() const {
    auto s = status_.load();
    return s == DownloadStatus::DOWNLOADING || s == DownloadStatus::FETCHING_HEADERS;
}

double Downloader::getSpeed() const {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - lastSpeedCheck_).count();

    if (elapsed < 100) return 0; // Too soon

    uint64_t currentBytes = 0;
    for (const auto& segment : segments_) {
        currentBytes += segment.getBytesDownloaded();
    }

    uint64_t bytesDelta = currentBytes - lastBytesDownloaded_.load();
    double speed = static_cast<double>(bytesDelta) / (elapsed / 1000.0);

    // Update for next check (mutable via const_cast for atomic)
    const_cast<std::atomic<uint64_t>&>(lastBytesDownloaded_).store(currentBytes);
    const_cast<std::chrono::steady_clock::time_point&>(lastSpeedCheck_) = now;

    return speed;
}

void Downloader::calculateSegments(int threadCount) {
    segments_.clear();

    if (totalSize_ == 0 || threadCount <= 1) {
        segments_.emplace_back(0, 0, totalSize_ > 0 ? totalSize_ - 1 : 0);
        return;
    }

    uint64_t segmentSize = totalSize_ / threadCount;

    for (int i = 0; i < threadCount; ++i) {
        uint64_t start = i * segmentSize;
        uint64_t end = (i == threadCount - 1) ? (totalSize_ - 1) : ((i + 1) * segmentSize - 1);
        segments_.emplace_back(i, start, end);
    }
}

void Downloader::joinThreads() {
    for (auto& thread : threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    threads_.clear();
}

} // namespace VelocityDM
