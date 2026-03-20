#pragma once
// ═══════════════════════════════════════════════════════
// SegmentWorker — Downloads a byte range of a file
// ═══════════════════════════════════════════════════════

#include <string>
#include <cstdint>
#include <atomic>

namespace VelocityDM {

enum class SegmentStatus {
    IDLE,
    DOWNLOADING,
    PAUSED,
    COMPLETED,
    ERROR
};

struct SegmentWorker {
    int id;                         // Worker ID (0-based)
    uint64_t startByte;             // Start of byte range
    uint64_t endByte;               // End of byte range
    std::atomic<uint64_t> currentPos;  // Current download position
    std::atomic<SegmentStatus> status; // Current status
    int retryCount;                 // Number of retries attempted
    std::string errorMessage;       // Last error message

    SegmentWorker()
        : id(0), startByte(0), endByte(0),
          currentPos(0), status(SegmentStatus::IDLE),
          retryCount(0) {}

    SegmentWorker(int workerId, uint64_t start, uint64_t end)
        : id(workerId), startByte(start), endByte(end),
          currentPos(start), status(SegmentStatus::IDLE),
          retryCount(0) {}

    // Get bytes downloaded in this segment
    uint64_t getBytesDownloaded() const {
        uint64_t pos = currentPos.load();
        return (pos > startByte) ? (pos - startByte) : 0;
    }

    // Get total bytes in this segment
    uint64_t getTotalBytes() const {
        return endByte - startByte + 1;
    }

    // Get progress percentage (0-100)
    double getProgress() const {
        uint64_t total = getTotalBytes();
        if (total == 0) return 100.0;
        return (static_cast<double>(getBytesDownloaded()) / total) * 100.0;
    }

    // Check if segment is complete
    bool isComplete() const {
        return currentPos.load() >= endByte;
    }
};

} // namespace VelocityDM
