cmake_minimum_required(VERSION 3.20)
project(VelocityDM VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find libcurl (via vcpkg or system)
find_package(CURL REQUIRED)

# Source files
set(CORE_SOURCES
    src/core/Downloader.cpp
    src/core/SegmentWorker.cpp
    src/core/FileIO.cpp
    src/core/StateManager.cpp
)

set(UI_SOURCES
    src/ui/MainWindow.cpp
)

# Main executable
add_executable(VelocityDM
    src/main.cpp
    ${CORE_SOURCES}
    ${UI_SOURCES}
    src/ui/resource.rc
)

target_include_directories(VelocityDM PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/vendor
)

target_link_libraries(VelocityDM PRIVATE
    CURL::libcurl
    comctl32     # Common controls (ListView)
    shell32      # Shell operations
    shlwapi      # Path utilities
    ole32        # COM (for tray icon)
)

# Browser Native Host (separate executable)
add_executable(VelocityDMHost
    src/browser_bridge/NativeHost.cpp
)

target_include_directories(VelocityDMHost PRIVATE
    ${CMAKE_SOURCE_DIR}/vendor
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(VelocityDMHost PRIVATE
    shell32
)

# Compiler options
if(MSVC)
    target_compile_options(VelocityDM PRIVATE /W4 /O2 /MT)
    target_compile_options(VelocityDMHost PRIVATE /W4 /O2 /MT)
endif()

# Output directories
set_target_properties(VelocityDM PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

set_target_properties(VelocityDMHost PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
