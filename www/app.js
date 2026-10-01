document.addEventListener("DOMContentLoaded", () => {
    console.log("🚀 JavaScript Frontend Engine Connected to C++ Backend!");

    // Function to fetch real-time telemetry from our C++ backend
    async function fetchServerStats() {
        try {
            const response = await fetch('/api/stats');
            if (!response.ok) throw new Error('Network error');
            
            const data = await response.json();
            
            // Dynamically update the HTML metrics in the browser window
            document.getElementById("server-runtime").innerText = data.runtime;
            document.getElementById("server-memory").innerText = data.memory_mode;
            document.getElementById("server-threads").innerText = data.active_workers;
        } catch (error) {
            console.error("❌ Failed to fetch stats from C++ engine:", error);
        }
    }

    // Run immediately on page load, then poll the server every 3 seconds
    fetchServerStats();
    setInterval(fetchServerStats, 3000);
});
