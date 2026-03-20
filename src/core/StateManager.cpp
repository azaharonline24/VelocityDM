// ═══════════════════════════════════════════════════════
// StateManager Implementation — JSON-based state persistence
// ═══════════════════════════════════════════════════════

#include "StateManager.h"
#include "FileIO.h"
#include <fstream>
#include <iostream>
#include <filesystem>

// nlohmann/json single header
#include "json.hpp"

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace VelocityDM {

std::string StateManager::getStatePath(const std::string& outputPath,
                                        const std::string& filename) {
    fs::path dir(outputPath);
    if (!fs::is_directory(dir)) {
        dir = dir.parent_path();
    }
    return (dir / (filename + ".velocity_state")).string();
}

bool StateManager::saveState(const std::string& outputPath,
                              const DownloadState& state) {
    try {
        std::string statePath = getStatePath(outputPath, state.filename);

        json j;
        j["url"] = state.url;
        j["filename"] = state.filename;
        j["outputPath"] = state.outputPath;
        j["totalSize"] = state.totalSize;
        j["supportsRange"] = state.supportsRange;
        j["segments"] = json::array();

        for (const auto& seg : state.segments) {
            json segmentJson;
            segmentJson["id"] = seg.id;
            segmentJson["startByte"] = seg.startByte;
            segmentJson["endByte"] = seg.endByte;
            segmentJson["downloaded"] = seg.downloaded;
            j["segments"].push_back(segmentJson);
        }

        std::ofstream file(statePath);
        if (!file.is_open()) {
            std::cerr << "[StateManager] Failed to save state: " << statePath << std::endl;
            return false;
        }

        file << j.dump(2); // Pretty print with 2-space indent
        file.close();

        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "[StateManager] Save error: " << e.what() << std::endl;
        return false;
    }
}

bool StateManager::loadState(const std::string& statePath,
                              DownloadState& state) {
    try {
        if (!FileIO::fileExists(statePath)) {
            return false;
        }

        std::ifstream file(statePath);
        if (!file.is_open()) {
            return false;
        }

        json j;
        file >> j;
        file.close();

        state.url = j["url"].get<std::string>();
        state.filename = j["filename"].get<std::string>();
        state.outputPath = j["outputPath"].get<std::string>();
        state.totalSize = j["totalSize"].get<uint64_t>();
        state.supportsRange = j["supportsRange"].get<bool>();

        state.segments.clear();
        for (const auto& segJson : j["segments"]) {
            SegmentState seg;
            seg.id = segJson["id"].get<int>();
            seg.startByte = segJson["startByte"].get<uint64_t>();
            seg.endByte = segJson["endByte"].get<uint64_t>();
            seg.downloaded = segJson["downloaded"].get<uint64_t>();
            state.segments.push_back(seg);
        }

        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "[StateManager] Load error: " << e.what() << std::endl;
        return false;
    }
}

bool StateManager::hasState(const std::string& outputPath,
                             const std::string& filename) {
    return FileIO::fileExists(getStatePath(outputPath, filename));
}

std::vector<std::string> StateManager::listStates(const std::string& directory) {
    std::vector<std::string> states;

    try {
        if (!fs::exists(directory) || !fs::is_directory(directory)) {
            return states;
        }

        for (const auto& entry : fs::directory_iterator(directory)) {
            if (entry.is_regular_file() &&
                entry.path().extension() == ".velocity_state") {
                states.push_back(entry.path().string());
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "[StateManager] List error: " << e.what() << std::endl;
    }

    return states;
}

bool StateManager::deleteState(const std::string& outputPath,
                                const std::string& filename) {
    try {
        std::string statePath = getStatePath(outputPath, filename);
        if (FileIO::fileExists(statePath)) {
            return fs::remove(statePath);
        }
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "[StateManager] Delete error: " << e.what() << std::endl;
        return false;
    }
}

} // namespace VelocityDM
