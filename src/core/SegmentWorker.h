#pragma once
// ═══════════════════════════════════════════════════════
// SegmentWorker — Downloads a byte range of a file
// ═══════════════════════════════════════════════════════

#include <string>
#include <cstdint>
#include <atomic>

namespace VelocityDM {

enum class SegmentStatus : int {
    IDLE = 0,
    DOWNLOADING = 1,
    PAUSED = 2,
    COMPLETED = 3,
    ERROR = 4
};

struct SegmentWorker {
    int id;
    uint64_t startByte;
    uint64_t endByte;
    std::atomic<uint64_t> currentPos;
    std::atomic<int> status;  // Stores SegmentStatus as int for MSVC compat
    int retryCount;
    std::string errorMessage;

    SegmentWorker()
        : id(0), startByte(0), endByte(0),
          currentPos(0), status(0), retryCount(0) {}

    SegmentWorker(int workerId, uint64_t start, uint64_t end)
        : id(workerId), startByte(start), endByte(end),
          currentPos(start), status(0), retryCount(0) {}

    SegmentStatus getStatus() const {
        return static_cast<SegmentStatus>(status.load());
    }

    void setStatus(SegmentStatus s) {
        status.store(static_cast<int>(s));
    }

    uint64_t getBytesDownloaded() const {
        uint64_t pos = currentPos.load();
        return (pos > startByte) ? (pos - startByte) : 0;
    }

    uint64_t getTotalBytes() const {
        return endByte - startByte + 1;
    }

    double getProgress() const {
        uint64_t total = getTotalBytes();
        if (total == 0) return 100.0;
        return (static_cast<double>(getBytesDownloaded()) / total) * 100.0;
    }

    bool isComplete() const {
        return currentPos.load() >= endByte;
    }
};

} // namespace VelocityDM
