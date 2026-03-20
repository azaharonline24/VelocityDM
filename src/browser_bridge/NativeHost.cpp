// ═══════════════════════════════════════════════════════
// NativeHost — Browser Native Messaging Host
// ═══════════════════════════════════════════════════════
// Reads messages from browser via stdin and forwards
// download URLs to VelocityDM main app via Named Pipe.

#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <windows.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Named pipe name for communicating with main app
const wchar_t* PIPE_NAME = L"\\\\.\\pipe\\VelocityDM";

// Read a message from stdin (Chrome Native Messaging format)
// Format: 4-byte length (uint32) + JSON payload
std::string readMessage() {
    // Read length (4 bytes, native byte order)
    uint32_t length = 0;
    if (!std::cin.read(reinterpret_cast<char*>(&length), 4)) {
        return "";
    }

    if (length == 0 || length > 1024 * 1024) { // Max 1MB
        return "";
    }

    // Read payload
    std::string payload(length, '\0');
    if (!std::cin.read(&payload[0], length)) {
        return "";
    }

    return payload;
}

// Write a message to stdout (Chrome Native Messaging format)
void writeMessage(const std::string& jsonStr) {
    uint32_t length = static_cast<uint32_t>(jsonStr.size());
    std::cout.write(reinterpret_cast<const char*>(&length), 4);
    std::cout.write(jsonStr.c_str(), length);
    std::cout.flush();
}

// Send message to main app via Named Pipe
bool sendToMainApp(const std::string& message) {
    HANDLE pipe = CreateFileW(
        PIPE_NAME,
        GENERIC_WRITE,
        0, nullptr,
        OPEN_EXISTING,
        0, nullptr
    );

    if (pipe == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD bytesWritten;
    BOOL success = WriteFile(
        pipe,
        message.c_str(),
        static_cast<DWORD>(message.size()),
        &bytesWritten,
        nullptr
    );

    CloseHandle(pipe);
    return success;
}

int main() {
    // Set binary mode for stdin/stdout
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);

    while (true) {
        std::string message = readMessage();
        if (message.empty()) {
            break; // EOF or error
        }

        try {
            json j = json::parse(message);
            std::string action = j.value("action", "");

            if (action == "download") {
                // Forward download to main app
                std::string url = j.value("url", "");
                std::string filename = j.value("filename", "");

                json response;
                response["status"] = "ok";

                if (!url.empty()) {
                    // Create a trigger file or use named pipe
                    // For now, write to a trigger file
                    std::string triggerPath = std::string(getenv("TEMP")) + "\\velocitydm_trigger.json";
                    std::ofstream trigger(triggerPath);
                    if (trigger.is_open()) {
                        json triggerData;
                        triggerData["action"] = "download";
                        triggerData["url"] = url;
                        triggerData["filename"] = filename;
                        trigger << triggerData.dump();
                        trigger.close();
                        response["message"] = "Download queued";
                    } else {
                        response["status"] = "error";
                        response["message"] = "Failed to create trigger";
                    }
                }

                writeMessage(response.dump());
            }
            else if (action == "ping") {
                json response;
                response["status"] = "ok";
                response["app"] = "VelocityDM";
                response["version"] = "1.0.0";
                writeMessage(response.dump());
            }
            else {
                json response;
                response["status"] = "error";
                response["message"] = "Unknown action: " + action;
                writeMessage(response.dump());
            }
        }
        catch (const std::exception& e) {
            json response;
            response["status"] = "error";
            response["message"] = std::string("Parse error: ") + e.what();
            writeMessage(response.dump());
        }
    }

    return 0;
}
