// VelocityDM Extension — Popup Script

document.addEventListener("DOMContentLoaded", () => {
    const statusDiv = document.getElementById("status");
    const urlInput = document.getElementById("urlInput");
    const downloadBtn = document.getElementById("downloadBtn");

    // Check connection status
    chrome.runtime.sendMessage({ action: "status" }, (response) => {
        if (response && response.connected) {
            statusDiv.className = "status connected";
            statusDiv.textContent = "✅ VelocityDM connected";
        } else {
            statusDiv.className = "status disconnected";
            statusDiv.textContent = "❌ VelocityDM not running";
        }
    });

    // Handle download button
    downloadBtn.addEventListener("click", () => {
        const url = urlInput.value.trim();
        if (!url) {
            alert("Please enter a URL");
            return;
        }

        chrome.runtime.sendMessage({
            action: "download",
            url: url,
            filename: ""
        }, (response) => {
            if (response && response.status === "sent") {
                urlInput.value = "";
                downloadBtn.textContent = "Sent! ✓";
                setTimeout(() => {
                    downloadBtn.textContent = "Download with VelocityDM";
                }, 2000);
            }
        });
    });

    // Enter key triggers download
    urlInput.addEventListener("keypress", (e) => {
        if (e.key === "Enter") {
            downloadBtn.click();
        }
    });
});
