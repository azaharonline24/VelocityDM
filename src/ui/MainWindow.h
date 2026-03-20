#pragma once
// ═══════════════════════════════════════════════════════
// MainWindow — Win32 GUI for managing downloads
// ═══════════════════════════════════════════════════════

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <memory>
#include "../core/Downloader.h"

#pragma comment(lib, "comctl32.lib")

namespace VelocityDM::UI {

// Window dimensions
constexpr int WINDOW_WIDTH = 900;
constexpr int WINDOW_HEIGHT = 600;

// Control IDs
constexpr int IDC_LISTVIEW = 1001;
constexpr int IDC_ADD_BTN = 1002;
constexpr int IDC_PAUSE_BTN = 1003;
constexpr int IDC_RESUME_BTN = 1004;
constexpr int IDC_DELETE_BTN = 1005;
constexpr int IDC_TIMER = 2001;

// ListView columns
enum Column {
    COL_FILENAME = 0,
    COL_SIZE,
    COL_PROGRESS,
    COL_SPEED,
    COL_STATUS,
    COL_COUNT
};

// Download entry in the list
struct DownloadEntry {
    std::unique_ptr<Downloader> downloader;
    std::string url;
    std::string filename;
    int listIndex;
};

class MainWindow {
public:
    MainWindow();
    ~MainWindow();

    // Create and show the window
    bool create(HINSTANCE hInstance);

    // Run the message loop
    int run();

    // Window procedure
    static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg,
                                     WPARAM wParam, LPARAM lParam);

private:
    HWND hwnd_;
    HWND listView_;
    HINSTANCE hInstance_;
    std::vector<DownloadEntry> downloads_;
    bool running_;

    // Initialize the ListView control
    void initListView();

    // Add a new download to the list
    void addDownload(const std::string& url, const std::string& outputPath);

    // Update all download progress
    void updateProgress();

    // Format file size to human readable
    static std::string formatSize(uint64_t bytes);

    // Format speed to human readable
    static std::string formatSpeed(double bytesPerSec);

    // Handle timer tick
    void onTimer();

    // Handle add download button
    void onAddDownload();

    // Handle pause selected
    void onPauseSelected();

    // Handle resume selected
    void onResumeSelected();

    // Handle delete selected
    void onDeleteSelected();

    // Get selected download index
    int getSelectedIndex() const;
};

// Entry point for GUI mode
int runMainWindow();

} // namespace VelocityDM::UI
