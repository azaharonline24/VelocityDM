// ═══════════════════════════════════════════════════════
// FileIO Implementation — Thread-safe file operations
// ═══════════════════════════════════════════════════════

#include "FileIO.h"
#include <iostream>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/statvfs.h>
#endif

namespace VelocityDM {

bool FileIO::createEmptyFile(const std::string& path, uint64_t size) {
    if (size == 0) return false;

    try {
        // Ensure parent directory exists
        fs::path filePath(path);
        if (filePath.has_parent_path()) {
            fs::create_directories(filePath.parent_path());
        }

        // Open file in binary mode
        std::ofstream file(path, std::ios::binary | std::ios::out);
        if (!file.is_open()) {
            std::cerr << "[FileIO] Failed to create file: " << path << std::endl;
            return false;
        }

        // Pre-allocate: seek to (size - 1) and write a null byte
        // This reserves the space on disk without filling RAM
        file.seekp(static_cast<std::streamoff>(size - 1));
        file.write("\0", 1);
        file.close();

        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "[FileIO] Exception creating file: " << e.what() << std::endl;
        return false;
    }
}

bool FileIO::writeAt(const std::string& path, uint64_t offset,
                     const char* data, size_t length) {
    std::lock_guard<std::mutex> lock(writeMutex_);

    try {
        // Open in binary mode for random access writes
        std::fstream file(path, std::ios::binary | std::ios::in | std::ios::out);
        if (!file.is_open()) {
            std::cerr << "[FileIO] Failed to open file for writing: " << path << std::endl;
            return false;
        }

        // Seek to the correct offset
        file.seekp(static_cast<std::streamoff>(offset));

        // Write data directly to disk (no RAM buffering of full file)
        file.write(data, static_cast<std::streamsize>(length));
        file.close();

        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "[FileIO] Write error: " << e.what() << std::endl;
        return false;
    }
}

bool FileIO::fileExists(const std::string& path) {
    return fs::exists(path) && fs::is_regular_file(path);
}

uint64_t FileIO::getFileSize(const std::string& path) {
    if (!fileExists(path)) return 0;
    return static_cast<uint64_t>(fs::file_size(path));
}

uint64_t FileIO::getFreeSpace(const std::string& path) {
    try {
#ifdef _WIN32
        ULARGE_INTEGER freeBytes;
        fs::path dirPath = fs::path(path).parent_path();
        if (!dirPath.has_root_path()) dirPath = fs::current_path();

        if (GetDiskFreeSpaceExW(dirPath.wstring().c_str(), &freeBytes, nullptr, nullptr)) {
            return freeBytes.QuadPart;
        }
        return 0;
#else
        struct statvfs stat;
        if (statvfs(path.c_str(), &stat) == 0) {
            return static_cast<uint64_t>(stat.f_bsize) * stat.f_bavail;
        }
        return 0;
#endif
    }
    catch (...) {
        return 0;
    }
}

bool FileIO::ensureDirectory(const std::string& path) {
    try {
        if (!fs::exists(path)) {
            return fs::create_directories(path);
        }
        return fs::is_directory(path);
    }
    catch (const std::exception& e) {
        std::cerr << "[FileIO] Failed to create directory: " << e.what() << std::endl;
        return false;
    }
}

std::string FileIO::extractFilename(const std::string& url,
                                     const std::string& outputPath) {
    // Extract filename from URL (last segment before query params)
    std::string filename;

    // Find last slash
    size_t lastSlash = url.rfind('/');
    if (lastSlash != std::string::npos) {
        filename = url.substr(lastSlash + 1);

        // Remove query parameters
        size_t queryPos = filename.find('?');
        if (queryPos != std::string::npos) {
            filename = filename.substr(0, queryPos);
        }

        // URL decode (basic: %20 -> space)
        std::string decoded;
        for (size_t i = 0; i < filename.size(); ++i) {
            if (filename[i] == '%' && i + 2 < filename.size()) {
                char hex[3] = { filename[i + 1], filename[i + 2], '\0' };
                char* endPtr;
                long val = strtol(hex, &endPtr, 16);
                if (*endPtr == '\0') {
                    decoded += static_cast<char>(val);
                    i += 2;
                    continue;
                }
            }
            decoded += filename[i];
        }
        filename = decoded;
    }

    // Fallback if no filename found
    if (filename.empty()) {
        filename = "download";
    }

    // Sanitize filename (remove invalid characters)
    const std::string invalidChars = "<>:\"/\\|?*";
    for (char& c : filename) {
        if (invalidChars.find(c) != std::string::npos) {
            c = '_';
        }
    }

    // Combine with output path
    if (!outputPath.empty()) {
        fs::path dir(outputPath);
        if (fs::is_directory(dir)) {
            return (dir / filename).string();
        }
    }

    return filename;
}

} // namespace VelocityDM
