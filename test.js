let slicerModule = null;
let stlData = null;

// Initialize the WASM module
async function initSlicer() {
    try {
        slicerModule = await import('./assets/js/slicer.js');
        console.log("Slicer WASM module loaded");
        document.getElementById('sliceButton').disabled = false;
    } catch (error) {
        console.error("Failed to load slicer module:", error);
        alert("Failed to load slicer. Please try again.");
    }
}

// Handle STL file upload
async function handleSTLUpload(event) {
    const file = event.target.files[0];
    if (!file) return;

    const reader = new FileReader();
    reader.onload = async function(e) {
        const arrayBuffer = e.target.result;
        stlData = new Uint8Array(arrayBuffer);
        document.getElementById('sliceButton').disabled = false;
        console.log("STL file loaded:", file.name);
    };
    reader.readAsArrayBuffer(file);
}

// Slice the loaded STL file
async function sliceModel() {
    if (!slicerModule || !stlData) {
        alert("Please load an STL file first");
        return;
    }

    try {
        // Allocate memory for STL data
        const stlDataPtr = slicerModule._malloc(stlData.length);
        slicerModule.HEAPU8.set(stlData, stlDataPtr);

        // Parse the STL
        const triangleCount = slicerModule._parseSTL(stlDataPtr, stlData.length);
        console.log(`Parsed STL with ${triangleCount} triangles`);

        // Free the memory
        slicerModule._free(stlDataPtr);

        if (triangleCount <= 0) {
            alert("Failed to parse STL file");
            return;
        }

        // Slice with default layer height (0.2mm)
        const layerHeight = 0.2;
        const gcode = slicerModule.getGcode(layerHeight);
        
        // Save the G-code
        saveGcode(gcode, "output.gcode");
        
    } catch (error) {
        console.error("Slicing failed:", error);
        alert("Slicing failed. See console for details.");
    }
}

// Save G-code to file
function saveGcode(content, filename) {
    const blob = new Blob([content], { type: 'text/plain' });
    const url = URL.createObjectURL(blob);
    
    const a = document.createElement('a');
    a.href = url;
    a.download = filename;
    document.body.appendChild(a);
    a.click();
    
    setTimeout(() => {
        document.body.removeChild(a);
        URL.revokeObjectURL(url);
    }, 100);
}

// Initialize when page loads
window.addEventListener('DOMContentLoaded', () => {
    initSlicer();
    
    document.getElementById('stlUpload').addEventListener('change', handleSTLUpload);
    document.getElementById('sliceButton').addEventListener('click', sliceModel);
});