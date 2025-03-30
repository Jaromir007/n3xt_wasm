import slicerModule from './assets/js/slicer.js';

let slicer;
let stlData = null;
let gcode = null;

// Initialize the slicer
slicerModule().then((module) => {
    slicer = module;
    console.log("WASM loaded");
    document.getElementById('status').textContent = "Ready to load STL";
    document.getElementById('sliceBtn').disabled = false;
}).catch((err) => {
    console.error("Failed to load WASM:", err);
    document.getElementById('status').textContent = "Failed to load slicer";
});

// Handle STL file upload
document.getElementById('stlFile').addEventListener('change', (e) => {
    const file = e.target.files[0];
    if (!file) return;

    const reader = new FileReader();
    reader.onload = (e) => {
        stlData = new Uint8Array(e.target.result);
        document.getElementById('status').textContent = `Loaded: ${file.name}`;
    };
    reader.readAsArrayBuffer(file);
});

// Slice button handler
document.getElementById('sliceBtn').addEventListener('click', () => {
    if (!slicer || !stlData) {
        alert("Please load an STL file first");
        return;
    }

    try {
        document.getElementById('status').textContent = "Slicing...";
        
        // Allocate and copy STL data
        const stlPtr = slicer._malloc(stlData.length);
        slicer.HEAPU8.set(stlData, stlPtr);

        // Parse STL
        const triangleCount = slicer._parseSTL(stlPtr, stlData.length);
        slicer._free(stlPtr);
        
        if (triangleCount <= 0) {
            throw new Error("No triangles found in STL");
        }

        // Get G-code (0.2mm layer height)
        gcode = slicer.getGcode(0.2);
        
        document.getElementById('status').textContent = "Slicing complete!";
        document.getElementById('downloadBtn').disabled = false;
    } catch (error) {
        console.error("Slicing failed:", error);
        document.getElementById('status').textContent = `Error: ${error.message}`;
    }
});

// Download button handler
document.getElementById('downloadBtn').addEventListener('click', () => {
    if (!gcode) return;
    
    const blob = new Blob([gcode], {type: 'text/plain'});
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = 'output.gcode';
    document.body.appendChild(a);
    a.click();
    setTimeout(() => {
        document.body.removeChild(a);
        URL.revokeObjectURL(url);
    }, 100);
});