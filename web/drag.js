(function() {
    const log = document.getElementById("log");
    const handle = document.getElementById("log-resize-handle");

    let isDragging = false;
    let startY = 0;
    let startHeight = 0;

    handle.addEventListener("mousedown", (e) => {
        isDragging = true;
        startY = e.clientY;
        startHeight = log.offsetHeight;
        document.body.style.userSelect = "none"; // evita selezione testo
    });

    document.addEventListener("mousemove", (e) => {
        if (!isDragging) return;

        const dy = startY - e.clientY;
        let newHeight = startHeight + dy;

        // Limiti min e max
        newHeight = Math.max(50, newHeight);
        newHeight = Math.min(window.innerHeight - 100, newHeight);

        log.style.height = newHeight + "px";
    });

    document.addEventListener("mouseup", () => {
        isDragging = false;
        document.body.style.userSelect = "auto";
    });
})();