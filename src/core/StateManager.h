#pragma once
// ═══════════════════════════════════════════════════════
// StateManager — Save/load download state for resume
// ═══════════════════════════════════════════════════════

#include <string>
#include <vector>
#include "SegmentWorker.h"

namespace VelocityDM {

struct SegmentState {
    int id;
    uint64_t startByte;
    uint64_t endByte;
    uint64_t downloaded;
};

struct DownloadState {
    std::string url;
    std::string filename;
    std::string outputPath;
    uint64_t totalSize;
    bool supportsRange;
    std::vector<SegmentState> segments;
};

class StateManager {
public:
    // Save download state to .velocity_state file
    static bool saveState(const std::string& outputPath,
                          const DownloadState& state);

    // Load download state from .velocity_state file
    static bool loadState(const std::string& statePath,
                          DownloadState& state);

    // Check if a state file exists for a given download
    static bool hasState(const std::string& outputPath,
                         const std::string& filename);

    // Get state file path for a download
    static std::string getStatePath(const std::string& outputPath,
                                     const std::string& filename);

    // List all saved states in a directory
    static std::vector<std::string> listStates(const std::string& directory);

    // Delete state file (after download completes)
    static bool deleteState(const std::string& outputPath,
                            const std::string& filename);
};

} // namespace VelocityDM
