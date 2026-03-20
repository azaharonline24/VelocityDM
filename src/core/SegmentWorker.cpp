// ═══════════════════════════════════════════════════════
// SegmentWorker Implementation — Per-thread download logic
// ═══════════════════════════════════════════════════════

#include "SegmentWorker.h"
#include "FileIO.h"
#include <curl/curl.h>
#include <iostream>
#include <thread>
#include <chrono>

namespace VelocityDM {

static constexpr int MAX_RETRIES = 3;

struct WriteContext {
    FileIO* fileIO;
    std::string outputPath;
    SegmentWorker* worker;
};

static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    WriteContext* ctx = static_cast<WriteContext*>(userp);

    uint64_t offset = ctx->worker->currentPos.load();

    if (!ctx->fileIO->writeAt(ctx->outputPath, offset,
                               static_cast<const char*>(contents), totalSize)) {
        return 0;
    }

    ctx->worker->currentPos.fetch_add(totalSize);
    return totalSize;
}

static int progressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow,
                             curl_off_t ultotal, curl_off_t ulnow) {
    SegmentWorker* worker = static_cast<SegmentWorker*>(clientp);

    if (worker->getStatus() == SegmentStatus::PAUSED) {
        return 1; // Abort
    }

    return 0; // Continue
}

void downloadSegment(const std::string& url,
                     SegmentWorker* worker,
                     FileIO* fileIO,
                     const std::string& outputPath) {
    worker->setStatus(SegmentStatus::DOWNLOADING);

    for (int attempt = 0; attempt <= MAX_RETRIES; ++attempt) {
        if (worker->getStatus() == SegmentStatus::PAUSED) {
            return;
        }

        CURL* curl = curl_easy_init();
        if (!curl) {
            worker->setStatus(SegmentStatus::ERROR);
            worker->errorMessage = "Failed to initialize CURL";
            return;
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        std::string range = "bytes=" +
            std::to_string(worker->currentPos.load()) + "-" +
            std::to_string(worker->endByte);
        curl_easy_setopt(curl, CURLOPT_RANGE, range.c_str());

        WriteContext ctx{ fileIO, outputPath, worker };
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ctx);

        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progressCallback);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, worker);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L);
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1L);
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 30L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "VelocityDM/1.0");

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res == CURLE_OK) {
            worker->setStatus(SegmentStatus::COMPLETED);
            return;
        }

        if (worker->getStatus() == SegmentStatus::PAUSED) {
            return;
        }

        worker->retryCount = attempt + 1;
        if (attempt < MAX_RETRIES) {
            std::cerr << "[Segment " << worker->id << "] Attempt "
                      << (attempt + 1) << " failed: "
                      << curl_easy_strerror(res) << ". Retrying..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1 << attempt));
        }
    }

    worker->setStatus(SegmentStatus::ERROR);
    worker->errorMessage = "Download failed after " +
        std::to_string(MAX_RETRIES) + " retries";
    std::cerr << "[Segment " << worker->id << "] ERROR: "
              << worker->errorMessage << std::endl;
}

} // namespace VelocityDM
