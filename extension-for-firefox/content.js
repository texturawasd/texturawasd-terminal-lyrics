console.log("[Terminal Lyrics] Content script loaded on", location.href);

// Inject a script directly into the page context to access navigator.mediaSession
const script = document.createElement('script');
script.textContent = `
console.log("[Terminal Lyrics Page] Injected script running");

setInterval(() => {
    try {
        let m = navigator.mediaSession.metadata;

        if (!m) {
            return;
        }

        const payload = {
            title: m.title || "",
            artist: m.artist || "",
            album: m.album || "",
            url: location.href
        };

        console.log("[Terminal Lyrics Page] Metadata found, posting:", payload);

        // Send message to content script via window.postMessage
        window.postMessage({
            type: "TERMINAL_LYRICS_METADATA",
            payload: payload
        }, "*");
    } catch (e) {
        console.log("[Terminal Lyrics Page] Error:", e.message);
    }
}, 1000);
`;

(document.head || document.documentElement).appendChild(script);
script.remove();

console.log("[Terminal Lyrics] Injected script loaded");

// Listen for messages from the injected script
window.addEventListener("message", (event) => {
    if (event.source !== window) return;

    if (event.data.type === "TERMINAL_LYRICS_METADATA") {
        console.log("[Terminal Lyrics] Received metadata:", event.data.payload);

        // Now make the fetch from content script (has proper permissions)
        fetch("http://127.0.0.1:3847/media", {
            method: "POST",
            headers: {
                "Content-Type": "application/json"
            },
            body: JSON.stringify(event.data.payload)
        }).then(response => {
            console.log("[Terminal Lyrics] POST status:", response.status);
        }).catch(error => {
            console.log("[Terminal Lyrics] POST error:", error.message);
        });
    }
});