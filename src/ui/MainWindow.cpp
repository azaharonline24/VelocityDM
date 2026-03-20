// ═══════════════════════════════════════════════════════
// MainWindow Implementation — Win32 download manager UI
// ═══════════════════════════════════════════════════════

#include "MainWindow.h"
#include <shlwapi.h>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <thread>

namespace fs = std::filesystem;

namespace VelocityDM::UI {

// Global instance for WndProc callback
static MainWindow* g_instance = nullptr;

MainWindow::MainWindow()
    : hwnd_(nullptr), listView_(nullptr), hInstance_(nullptr), running_(false) {
    g_instance = this;
}

MainWindow::~MainWindow() {
    if (hwnd_) {
        KillTimer(hwnd_, IDC_TIMER);
        DestroyWindow(hwnd_);
    }
    g_instance = nullptr;
}

bool MainWindow::create(HINSTANCE hInstance) {
    hInstance_ = hInstance;

    // Register window class
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = wndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = L"VelocityDMClass";
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);

    if (!RegisterClassEx(&wc)) {
        return false;
    }

    // Create main window
    hwnd_ = CreateWindowEx(
        0,
        L"VelocityDMClass",
        L"VelocityDM - Download Manager",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        nullptr, nullptr, hInstance, this
    );

    if (!hwnd_) {
        return false;
    }

    // Initialize common controls
    INITCOMMONCONTROLSEX icex = {};
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);

    // Create ListView
    listView_ = CreateWindowEx(
        0, WC_LISTVIEW, L"",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
        10, 50, WINDOW_WIDTH - 40, WINDOW_HEIGHT - 120,
        hwnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_LISTVIEW)),
        hInstance, nullptr
    );

    initListView();

    // Create toolbar buttons
    CreateWindow(L"BUTTON", L"+ Add Download",
                 WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                 10, 10, 120, 30, hwnd_,
                 reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_ADD_BTN)),
                 hInstance, nullptr);

    CreateWindow(L"BUTTON", L"⏸ Pause",
                 WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                 140, 10, 80, 30, hwnd_,
                 reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_PAUSE_BTN)),
                 hInstance, nullptr);

    CreateWindow(L"BUTTON", L"▶ Resume",
                 WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                 230, 10, 80, 30, hwnd_,
                 reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_RESUME_BTN)),
                 hInstance, nullptr);

    CreateWindow(L"BUTTON", L"🗑 Delete",
                 WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                 320, 10, 80, 30, hwnd_,
                 reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_DELETE_BTN)),
                 hInstance, nullptr);

    // Show window
    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);

    // Start update timer (100ms as per spec)
    SetTimer(hwnd_, IDC_TIMER, 100, nullptr);

    return true;
}

int MainWindow::run() {
    running_ = true;
    MSG msg = {};

    // Main message loop — yields when idle (0% CPU as per spec)
    while (running_) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running_ = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            // Sleep when idle to keep CPU at 0%
            WaitMessage();
        }
    }

    return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK MainWindow::wndProc(HWND hwnd, UINT msg,
                                       WPARAM wParam, LPARAM lParam) {
    if (!g_instance) {
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    switch (msg) {
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_ADD_BTN:
                    g_instance->onAddDownload();
                    break;
                case IDC_PAUSE_BTN:
                    g_instance->onPauseSelected();
                    break;
                case IDC_RESUME_BTN:
                    g_instance->onResumeSelected();
                    break;
                case IDC_DELETE_BTN:
                    g_instance->onDeleteSelected();
                    break;
            }
            break;

        case WM_TIMER:
            if (wParam == IDC_TIMER) {
                g_instance->onTimer();
            }
            break;

        case WM_SIZE: {
            // Resize ListView to fill window
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);
            if (g_instance->listView_) {
                SetWindowPos(g_instance->listView_, nullptr,
                             10, 50, width - 40, height - 120, SWP_NOZORDER);
            }
            break;
        }

        case WM_DESTROY:
            g_instance->running_ = false;
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0;
}

void MainWindow::initListView() {
    // Set extended styles
    ListView_SetExtendedListViewStyle(listView_,
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    // Add columns
    LVCOLUMN col = {};
    col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

    col.cx = 300;
    col.pszText = const_cast<LPWSTR>(L"Filename");
    ListView_InsertColumn(listView_, COL_FILENAME, &col);

    col.cx = 100;
    col.pszText = const_cast<LPWSTR>(L"Size");
    ListView_InsertColumn(listView_, COL_SIZE, &col);

    col.cx = 200;
    col.pszText = const_cast<LPWSTR>(L"Progress");
    ListView_InsertColumn(listView_, COL_PROGRESS, &col);

    col.cx = 100;
    col.pszText = const_cast<LPWSTR>(L"Speed");
    ListView_InsertColumn(listView_, COL_SPEED, &col);

    col.cx = 100;
    col.pszText = const_cast<LPWSTR>(L"Status");
    ListView_InsertColumn(listView_, COL_STATUS, &col);
}

void MainWindow::addDownload(const std::string& url,
                              const std::string& outputPath) {
    auto entry = std::make_unique<DownloadEntry>();
    entry->downloader = std::make_unique<Downloader>();
    entry->url = url;
    entry->filename = FileIO::extractFilename(url, outputPath);

    // Add to ListView
    LVITEM item = {};
    item.mask = LVIF_TEXT;
    item.iItem = static_cast<int>(downloads_.size());

    std::wstring wFilename(entry->filename.begin(), entry->filename.end());
    item.pszText = const_cast<LPWSTR>(wFilename.c_str());
    entry->listIndex = ListView_InsertItem(listView_, &item);

    // Start download
    entry->downloader->start(url, outputPath);

    downloads_.push_back(std::move(entry));
}

void MainWindow::updateProgress() {
    for (size_t i = 0; i < downloads_.size(); ++i) {
        auto& entry = downloads_[i];
        if (!entry->downloader) continue;

        auto info = entry->downloader->getInfo();

        // Update size column
        std::wstring wSize = std::wstring(
            formatSize(info.totalSize).begin(),
            formatSize(info.totalSize).end());
        ListView_SetItemText(listView_, static_cast<int>(i), COL_SIZE,
                             const_cast<LPWSTR>(wSize.c_str()));

        // Update progress column
        std::wstringstream progressStr;
        progressStr << std::fixed << std::setprecision(1) << info.progress << L"%";
        ListView_SetItemText(listView_, static_cast<int>(i), COL_PROGRESS,
                             const_cast<LPWSTR>(progressStr.str().c_str()));

        // Update speed column
        std::wstring wSpeed = std::wstring(
            formatSpeed(info.speed).begin(),
            formatSpeed(info.speed).end());
        ListView_SetItemText(listView_, static_cast<int>(i), COL_SPEED,
                             const_cast<LPWSTR>(wSpeed.c_str()));

        // Update status column
        std::string statusStr;
        switch (info.status) {
            case DownloadStatus::DOWNLOADING: statusStr = "Downloading"; break;
            case DownloadStatus::PAUSED: statusStr = "Paused"; break;
            case DownloadStatus::COMPLETED: statusStr = "Completed"; break;
            case DownloadStatus::ERROR: statusStr = "Error"; break;
            default: statusStr = "Idle"; break;
        }
        std::wstring wStatus(statusStr.begin(), statusStr.end());
        ListView_SetItemText(listView_, static_cast<int>(i), COL_STATUS,
                             const_cast<LPWSTR>(wStatus.c_str()));
    }
}

std::string MainWindow::formatSize(uint64_t bytes) {
    const char* units[] = { "B", "KB", "MB", "GB", "TB" };
    int unit = 0;
    double size = static_cast<double>(bytes);

    while (size >= 1024.0 && unit < 4) {
        size /= 1024.0;
        unit++;
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << size << " " << units[unit];
    return oss.str();
}

std::string MainWindow::formatSpeed(double bytesPerSec) {
    if (bytesPerSec <= 0) return "—";

    const char* units[] = { "B/s", "KB/s", "MB/s", "GB/s" };
    int unit = 0;
    double speed = bytesPerSec;

    while (speed >= 1024.0 && unit < 3) {
        speed /= 1024.0;
        unit++;
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << speed << " " << units[unit];
    return oss.str();
}

void MainWindow::onTimer() {
    updateProgress();
}

void MainWindow::onAddDownload() {
    // Simple URL input dialog
    wchar_t urlBuffer[2048] = {};
    wchar_t pathBuffer[MAX_PATH] = {};

    // Get URL
    if (GetDlgItemText(hwnd_, IDC_ADD_BTN, urlBuffer, 2048) == 0) {
        // Show input dialog
        // For now, use a simple message box
        // In a full implementation, this would be a proper dialog
        MessageBox(hwnd_, L"Enter URL in the format:\nVelocityDM.exe <url> <path>",
                   L"Add Download", MB_OK | MB_ICONINFORMATION);
        return;
    }

    // Get output path (default to Downloads folder)
    std::string outputPath = (fs::path(getenv("USERPROFILE")) / "Downloads").string();
    std::string url(urlBuffer, urlBuffer + wcslen(urlBuffer));

    addDownload(url, outputPath);
}

void MainWindow::onPauseSelected() {
    int idx = getSelectedIndex();
    if (idx >= 0 && idx < static_cast<int>(downloads_.size())) {
        downloads_[idx]->downloader->pause();
    }
}

void MainWindow::onResumeSelected() {
    int idx = getSelectedIndex();
    if (idx >= 0 && idx < static_cast<int>(downloads_.size())) {
        downloads_[idx]->downloader->resume();
    }
}

void MainWindow::onDeleteSelected() {
    int idx = getSelectedIndex();
    if (idx >= 0 && idx < static_cast<int>(downloads_.size())) {
        downloads_[idx]->downloader->cancel();
        ListView_DeleteItem(listView_, idx);
        downloads_.erase(downloads_.begin() + idx);
    }
}

int MainWindow::getSelectedIndex() const {
    return ListView_GetNextItem(listView_, -1, LVNI_SELECTED);
}

// Entry point for GUI mode
int runMainWindow() {
    MainWindow window;
    HINSTANCE hInstance = GetModuleHandle(nullptr);

    if (!window.create(hInstance)) {
        MessageBox(nullptr, L"Failed to create window", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    return window.run();
}

} // namespace VelocityDM::UI
