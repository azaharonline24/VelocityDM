// ═══════════════════════════════════════════════════════
// VelocityDM Browser Extension — Background Script
// ═══════════════════════════════════════════════════════
// Intercepts browser downloads and sends URLs to VelocityDM
// via Chrome Native Messaging.

const NATIVE_HOST_NAME = "com.velocitydm";

// File extensions to intercept (not HTML/PHP pages)
const DOWNLOAD_EXTENSIONS = [
    ".zip", ".rar", ".7z", ".tar", ".gz", ".bz2",
    ".exe", ".msi", ".dmg", ".pkg", ".deb", ".rpm",
    ".iso", ".img", ".bin",
    ".mp4", ".mkv", ".avi", ".mov", ".wmv", ".flv",
    ".mp3", ".flac", ".wav", ".aac", ".ogg",
    ".pdf", ".doc", ".docx", ".xls", ".xlsx", ".ppt", ".pptx",
    ".jpg", ".jpeg", ".png", ".gif", ".bmp", ".webp", ".svg",
    ".apk", ".ipa",
    ".torrent"
];

// Check if URL should be intercepted
function shouldIntercept(url, filename) {
    if (!url || !url.startsWith("http")) return false;

    // Check file extension
    const lower = (filename || url).toLowerCase();
    return DOWNLOAD_EXTENSIONS.some(ext => lower.endsWith(ext));
}

// Send URL to native host
function sendToVelocityDM(url, filename) {
    const message = {
        action: "download",
        url: url,
        filename: filename || ""
    };

    chrome.runtime.sendNativeMessage(NATIVE_HOST_NAME, message, (response) => {
        if (chrome.runtime.lastError) {
            console.error("VelocityDM: Native host error:",
                          chrome.runtime.lastError.message);
            return;
        }
        console.log("VelocityDM: Response:", response);
    });
}

// Listen for new downloads
chrome.downloads.onCreated.addListener((downloadItem) => {
    const url = downloadItem.url;
    const filename = downloadItem.filename;

    if (shouldIntercept(url, filename)) {
        console.log("VelocityDM: Intercepting download:", filename || url);

        // Cancel browser download
        chrome.downloads.cancel(downloadItem.id, () => {
            console.log("VelocityDM: Browser download cancelled");

            // Send to VelocityDM
            sendToVelocityDM(url, filename);

            // Show notification
            chrome.notifications.create({
                type: "basic",
                iconUrl: "icon48.png",
                title: "VelocityDM",
                message: `Downloading with VelocityDM: ${filename || url}`
            });
        });
    }
});

// Listen for messages from popup
chrome.runtime.onMessage.addListener((message, sender, sendResponse) => {
    if (message.action === "download") {
        sendToVelocityDM(message.url, message.filename);
        sendResponse({ status: "sent" });
    } else if (message.action === "status") {
        // Check native host connection
        chrome.runtime.sendNativeMessage(NATIVE_HOST_NAME, { action: "ping" }, (response) => {
            if (chrome.runtime.lastError) {
                sendResponse({ connected: false, error: chrome.runtime.lastError.message });
            } else {
                sendResponse({ connected: true, response });
            }
        });
        return true; // Async response
    }
});

console.log("VelocityDM: Extension loaded");
