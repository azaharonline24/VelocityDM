#pragma once
// ═══════════════════════════════════════════════════════
// FileIO — Thread-safe file operations for download segments
// ═══════════════════════════════════════════════════════

#include <string>
#include <fstream>
#include <mutex>
#include <filesystem>
#include <cstdint>

namespace fs = std::filesystem;

namespace VelocityDM {

class FileIO {
public:
    // Pre-allocate file on disk to prevent fragmentation
    // Writes a null byte at (size - 1) to reserve space
    static bool createEmptyFile(const std::string& path, uint64_t size);

    // Write data at specific offset (thread-safe)
    // Uses mutex to prevent concurrent writes
    bool writeAt(const std::string& path, uint64_t offset,
                 const char* data, size_t length);

    // Check if file exists
    static bool fileExists(const std::string& path);

    // Get file size
    static uint64_t getFileSize(const std::string& path);

    // Get free disk space for a given path
    static uint64_t getFreeSpace(const std::string& path);

    // Ensure directory exists (create if needed)
    static bool ensureDirectory(const std::string& path);

    // Extract filename from URL
    static std::string extractFilename(const std::string& url,
                                        const std::string& outputPath);

private:
    std::mutex writeMutex_;
};

} // namespace VelocityDM
