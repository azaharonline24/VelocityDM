// ═══════════════════════════════════════════════════════
// SegmentWorker Implementation — Per-thread download logic
// ═══════════════════════════════════════════════════════

#include "SegmentWorker.h"
#include "FileIO.h"
#include <curl/curl.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>

namespace VelocityDM {

// Maximum retry attempts per segment
static constexpr int MAX_RETRIES = 3;

// Write callback for libcurl — writes directly to file at correct offset
struct WriteContext {
    FileIO* fileIO;
    std::string outputPath;
    SegmentWorker* worker;
};

static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    WriteContext* ctx = static_cast<WriteContext*>(userp);

    // Get current write position
    uint64_t offset = ctx->worker->currentPos.load();

    // Write directly to disk (low RAM usage)
    if (!ctx->fileIO->writeAt(ctx->outputPath, offset,
                               static_cast<const char*>(contents), totalSize)) {
        return 0; // Signal error to CURL
    }

    // Update position
    ctx->worker->currentPos.fetch_add(totalSize);

    return totalSize;
}

// Progress callback for libcurl
static int progressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow,
                             curl_off_t ultotal, curl_off_t ulnow) {
    SegmentWorker* worker = static_cast<SegmentWorker*>(clientp);

    // Check if paused
    if (worker->status.load() == SegmentStatus::PAUSED) {
        return 1; // Abort transfer
    }

    return 0; // Continue
}

// Download a segment (called in a thread)
void downloadSegment(const std::string& url,
                     SegmentWorker* worker,
                     FileIO* fileIO,
                     const std::string& outputPath) {
    worker->status.store(SegmentStatus::DOWNLOADING);

    for (int attempt = 0; attempt <= MAX_RETRIES; ++attempt) {
        if (worker->status.load() == SegmentStatus::PAUSED) {
            return; // User paused
        }

        CURL* curl = curl_easy_init();
        if (!curl) {
            worker->status.store(SegmentStatus::ERROR);
            worker->errorMessage = "Failed to initialize CURL";
            return;
        }

        // Set URL
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        // Set byte range: "bytes=start-end"
        std::string range = "bytes=" +
            std::to_string(worker->currentPos.load()) + "-" +
            std::to_string(worker->endByte);
        curl_easy_setopt(curl, CURLOPT_RANGE, range.c_str());

        // Set write callback
        WriteContext ctx{ fileIO, outputPath, worker };
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ctx);

        // Set progress callback
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progressCallback);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, worker);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

        // Timeouts (per strict rules)
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L);
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1L);
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 30L);

        // Follow redirects
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);

        // User agent
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "VelocityDM/1.0");

        // Perform download
        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res == CURLE_OK) {
            worker->status.store(SegmentStatus::COMPLETED);
            return;
        }

        // Check if paused (not an error)
        if (worker->status.load() == SegmentStatus::PAUSED) {
            return;
        }

        // Retry logic (exactly 3 retries, no infinite loop)
        worker->retryCount = attempt + 1;
        if (attempt < MAX_RETRIES) {
            std::cerr << "[Segment " << worker->id << "] Attempt "
                      << (attempt + 1) << " failed: "
                      << curl_easy_strerror(res) << ". Retrying..." << std::endl;

            // Wait before retry (exponential backoff)
            std::this_thread::sleep_for(
                std::chrono::seconds(1 << attempt)); // 1s, 2s, 4s
        }
    }

    // All retries exhausted
    worker->status.store(SegmentStatus::ERROR);
    worker->errorMessage = "Download failed after " +
        std::to_string(MAX_RETRIES) + " retries";
    std::cerr << "[Segment " << worker->id << "] ERROR: "
              << worker->errorMessage << std::endl;
}

} // namespace VelocityDM
