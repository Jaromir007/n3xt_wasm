import * as THREE from "three";
import { STLLoader } from "three/examples/jsm/loaders/STLLoader";
import { OrbitControls } from "three/examples/jsm/controls/OrbitControls";
import slicerModule from "./slicer.js";

class Config {
    // Print bed
    static PRINT_BED_SIZE = new THREE.Vector2(250, 210);
    static PRINT_BED_HEIGHT = 1;
    static AXIS_SIZE = 15;
    static PRINT_BED_COLOR = 0x111111;
    static LINE_COLOR = 0x666666;

    // Light
    static LIGHT_COLOR = 0xffffff;
    static LIGHT_INTENSITY = 0.5;
    static HEMISPHERE_LIGHT_COLOR = 0x444444;
    static HEMISPHERE_LIGHT_INTENSITY = 0.5;
    static LIGHT_POSITION = { x: 0, y: 250, z: 0 };

    // Model
    static MODEL_COLOR = 0xff8c00;
    static MODEL_COLOR_SELECTED = 0x00ff00;
    static CORNER_COLOR = 0xffffff;
    static EDGE_COLOR = 0xffffff;
    static EDGE_WIDTH = 1;
}

class PrintBed {
    constructor() {
        this.size = Config.PRINT_BED_SIZE;
        this.height = Config.PRINT_BED_HEIGHT;

        this.color = Config.PRINT_BED_COLOR;
        this.axisSize = Config.AXIS_SIZE;

        this.xAxis = null;
        this.yAxis = null;
        this.zAxis = null;
        this.bed = null;

        this.init();
    }

    init() {
        const loader = new STLLoader();
        loader.load('./assets/stl/prusa_bed.stl', (geometry) => {
            geometry.center();
            const material = new THREE.MeshStandardMaterial({
                color: this.color,
                roughness: 1,
                metalness: 0.1
            });

            this.bed = new THREE.Mesh(geometry, material);
            this.bed.position.y = -this.height / 2;
            this.bed.position.x = 0;
            scene.add(this.bed);

            // Outline
            const outlineMaterial = new THREE.LineBasicMaterial({ color: 0xffffff });
            const outlineGeometry = new THREE.BufferGeometry().setFromPoints([
                new THREE.Vector3(-this.size.x / 2, 0, -this.size.y / 2),
                new THREE.Vector3(this.size.x / 2, 0, -this.size.y / 2),
                new THREE.Vector3(this.size.x / 2, 0, this.size.y / 2),
                new THREE.Vector3(-this.size.x / 2, 0, this.size.y / 2),
                new THREE.Vector3(-this.size.x / 2, 0, -this.size.y / 2)
            ]);
            const outline = new THREE.Line(outlineGeometry, outlineMaterial);
            scene.add(outline);

            // Grid
            const gridMaterial = new THREE.LineBasicMaterial({ color: 0x666666 });
            const gridGeometry = new THREE.BufferGeometry();
            const gridVertices = [];

            const sizeX = this.size.x - 0.4;
            const sizeY = this.size.y - 10;

            const stepX = sizeX / 5;
            const stepY = sizeY / 4;

            for (let i = -sizeX / 2; i <= sizeX / 2; i += stepX) {
                gridVertices.push(i, 0, -sizeY / 2, i, 0, sizeY / 2);
            }
            for (let j = -sizeY / 2; j <= sizeY / 2; j += stepY) {
                gridVertices.push(-sizeX / 2, 0, j, sizeX / 2, 0, j);
            }

            gridGeometry.setAttribute('position', new THREE.Float32BufferAttribute(gridVertices, 3));
            const grid = new THREE.LineSegments(gridGeometry, gridMaterial);
            scene.add(grid);

        });

        // X axis (red)
        const xAxisMaterial = new THREE.MeshStandardMaterial({ color: 0xff0000 });
        const xAxisGeometry = new THREE.BoxGeometry(this.axisSize, 1, 1);
        this.xAxis = new THREE.Mesh(xAxisGeometry, xAxisMaterial);
        this.xAxis.position.set(this.axisSize / 2 - this.size.x / 2, 0.5, this.size.y / 2);

        // Y axis (green)
        const yAxisMaterial = new THREE.MeshStandardMaterial({ color: 0x00ff00 });
        const yAxisGeometry = new THREE.BoxGeometry(1, 1, this.axisSize);
        this.yAxis = new THREE.Mesh(yAxisGeometry, yAxisMaterial);
        this.yAxis.position.set(-this.size.x / 2, 0.5, this.size.y / 2 - this.axisSize / 2);

        // Z axis (blue)
        const zAxisMaterial = new THREE.MeshStandardMaterial({ color: 0x0000ff });
        const zAxisGeometry = new THREE.BoxGeometry(1, this.axisSize, 1);
        this.zAxis = new THREE.Mesh(zAxisGeometry, zAxisMaterial);
        this.zAxis.position.set(-this.size.x / 2, this.axisSize / 2, this.size.y / 2);
    }

    render() {
        scene.add(this.xAxis);
        scene.add(this.yAxis);
        scene.add(this.zAxis);
    }
}

class Light {
    constructor() {
        this.color = Config.LIGHT_COLOR;
        this.position = Config.LIGHT_POSITION;
        this.light = new THREE.DirectionalLight(Config.LIGHT_COLOR, Config.LIGHT_INTENSITY);
        this.hemisphereLight = new THREE.HemisphereLight(Config.LIGHT_COLOR, Config.HEMISPHERE_LIGHT_COLOR, Config.HEMISPHERE_LIGHT_INTENSITY);

        this.init();
    }

    init() {
        this.light.position.set(this.position.x, this.position.y, this.position.z);
        this.hemisphereLight.position.set(this.position.x, this.position.y, this.position.z);

        // default light
        const ambientLight = new THREE.AmbientLight(0x404040, 0.3);
        scene.add(ambientLight);

        const spotLight = new THREE.SpotLight(0xffffff, 0.3);
        spotLight.position.set(-250, 250, -250);
        spotLight.angle = Math.PI / 6;
        spotLight.penumbra = 0.1;
        spotLight.decay = 2;
        spotLight.distance = 250;
        spotLight.castShadow = true;
        scene.add(spotLight);

        const hemiLight = new THREE.HemisphereLight(0xffffbb, 0x080820, 0.5);
        scene.add(hemiLight);
    }

    render() {
        scene.add(this.light);
        scene.add(this.hemisphereLight);
    }
}

class Model extends THREE.Mesh {
    constructor(geometry, material, name) {
        super(geometry, material);
        this.name = name;
        this.selected = false;
        this.boundingBox = null;
        this.hasBoundingBox = false;

        this.color = Config.MODEL_COLOR;
        this.colorSelected = Config.MODEL_COLOR_SELECTED;

        this.material = new THREE.MeshStandardMaterial({
            color: this.color,
            roughness: 0.6,
            metalness: 0.3
        });

        this.corners = [];
        this.edges = [];
    }

    createBoundingBox() {
        const box = new THREE.Box3().setFromObject(this);
        const cornerGeometry = new THREE.SphereGeometry(0.5, 8, 8);
        const cornerMaterial = new THREE.MeshBasicMaterial({ color: 0xffffff });

        this.corners = [
            new THREE.Vector3(box.min.x, box.min.y, box.min.z),
            new THREE.Vector3(box.min.x, box.min.y, box.max.z),
            new THREE.Vector3(box.min.x, box.max.y, box.min.z),
            new THREE.Vector3(box.min.x, box.max.y, box.max.z),
            new THREE.Vector3(box.max.x, box.min.y, box.min.z),
            new THREE.Vector3(box.max.x, box.min.y, box.max.z),
            new THREE.Vector3(box.max.x, box.max.y, box.min.z),
            new THREE.Vector3(box.max.x, box.max.y, box.max.z)
        ].map(corner => {
            const cornerMesh = new THREE.Mesh(cornerGeometry, cornerMaterial);
            cornerMesh.position.copy(corner);
            scene.add(cornerMesh);
            return cornerMesh;
        });

        const edgeMaterial = new THREE.LineBasicMaterial({ color: 0xffffff });

        this.edges = [
            [this.corners[0], this.corners[1]],
            [this.corners[0], this.corners[2]],
            [this.corners[0], this.corners[4]],
            [this.corners[1], this.corners[3]],
            [this.corners[1], this.corners[5]],
            [this.corners[2], this.corners[3]],
            [this.corners[2], this.corners[6]],
            [this.corners[3], this.corners[7]],
            [this.corners[4], this.corners[5]],
            [this.corners[4], this.corners[6]],
            [this.corners[5], this.corners[7]],
            [this.corners[6], this.corners[7]]
        ].map(([start, end]) => {
            const edgeGeometry = new THREE.BufferGeometry();
            const vertices = new Float32Array([
                start.position.x, start.position.y, start.position.z,
                end.position.x, end.position.y, end.position.z
            ]);
            edgeGeometry.setAttribute("position", new THREE.BufferAttribute(vertices, 3));
            const edge = new THREE.Line(edgeGeometry, edgeMaterial);
            scene.add(edge);
            return edge;
        });

        this.hasBoundingBox = true;
    }

    removeBoundingBox() {
        if (!this.corners.length) return;

        this.corners.forEach(corner => {
            scene.remove(corner);
            corner.geometry.dispose();
            corner.material.dispose();
        });

        this.edges.forEach(edge => {
            scene.remove(edge);
        });

        this.corners = [];
        this.edges = [];

        this.hasBoundingBox = false;
    }

    setColor(color) {
        this.material.color.set(color);
    }

    clicked() {
        this.selected = !this.selected;
        this.boundingBox = (this.selected ? this.createBoundingBox() : this.removeBoundingBox());
        this.setColor(this.selected ? this.colorSelected : this.color);
    }
}



// -------------------------------------------------------------



function init() {
    scene = new THREE.Scene();
    scene.background = new THREE.Color(0xe0e0e0);

    camera = new THREE.PerspectiveCamera(75, window.innerWidth / window.innerHeight, 0.1, 1000);
    camera.position.set(0, 225, 150);

    const slicerCanvas = document.getElementById("slicerCanvas");
    renderer = new THREE.WebGLRenderer({
        antialias: true,
        powerPreference: "high-performance",
        canvas: slicerCanvas
    });

    renderer.setSize(window.innerWidth, window.innerHeight);
    renderer.setPixelRatio(window.devicePixelRatio);

    controls = new OrbitControls(camera, renderer.domElement);
    controls.enableDamping = true;
    controls.screenSpacePanning = false;
    controls.mouseButtons = {
        LEFT: THREE.MOUSE.ROTATE,
        MIDDLE: THREE.MOUSE.DOLLY,
        RIGHT: THREE.MOUSE.PAN
    };

    printBed = new PrintBed();
    light = new Light();

    printBed.render();
    light.render();

    document.getElementById("clearButton").addEventListener("click", clearAll);

    renderer.domElement.addEventListener("click", selectObject, false);
    window.addEventListener("keydown", cameraPosition);

    animate();
}

function cameraPosition(event) {
    switch (event.key.toLowerCase()) {
        case "b": // Reset 
            controls.reset();
            camera.position.set(0, 225, 150);
            break;
        case "z": // Zoom out 
            zoomToScene();
            break;
        case "0": // Isometric view
            camera.position.set(150, 225, 150);
            break;
        case "1": // Top-down view
            camera.position.set(0, 225, 0);
            break;
        case "2": // Bottom-up view
            camera.position.set(0, 0, 250);
            break;
        case "3": // Front view
            camera.position.set(0, 225, 150);
            break;
        case "4": // Back view
            camera.position.set(0, 225, -150);
            break;
        case "5": // Left view
            camera.position.set(-225, 150, 0);
            break;
        case "6": // Right view
            camera.position.set(225, 150, 0);
            break;
        case "i": // Zoom in
            camera.position.addScaledVector(camera.getWorldDirection(new THREE.Vector3()), 10);
            break;
        case "o": // Zoom out
            camera.position.addScaledVector(camera.getWorldDirection(new THREE.Vector3()), -10);
            break;
    }
    controls.update();
}

function zoomToScene() {
    const box = new THREE.Box3().setFromObject(scene);
    if (!box.isEmpty()) {
        const center = box.getCenter(new THREE.Vector3());
        const size = box.getSize(new THREE.Vector3()).length();
        camera.position.set(center.x, center.y + size, center.z + size);
        controls.target.copy(center);
    }
    controls.update();
}

function animate() {
    requestAnimationFrame(animate);
    controls.update();
    renderer.render(scene, camera);
}

function displaySTL(event) {
    const file = event.dataTransfer ? event.dataTransfer.files[0] : event.target.files[0];
    if (!file) return;

    const reader = new FileReader();
    reader.onload = function (e) {
        const loader = new STLLoader();
        const geometry = loader.parse(e.target.result);

        geometry.computeBoundingBox();
        const bbox = geometry.boundingBox;

        const center = new THREE.Vector3();
        bbox.getCenter(center);
        const size = new THREE.Vector3();
        bbox.getSize(size);

        geometry.translate(-center.x, -center.y, -center.z);

        const material = new THREE.MeshStandardMaterial({ color: 0xffffff });
        const model = new Model(geometry, material, file.name);

        model.rotation.set(-Math.PI / 2, 0, 0);
        model.position.y = -bbox.min.z;

        scene.add(model);
        imported.push(model);
    };
    reader.readAsArrayBuffer(file);
}

function selectObject(event) {
    event.preventDefault();
    if (sliced) return;

    const mouse = new THREE.Vector2();
    mouse.x = (event.clientX / window.innerWidth) * 2 - 1;
    mouse.y = -(event.clientY / window.innerHeight) * 2 + 1;

    const raycaster = new THREE.Raycaster();
    raycaster.setFromCamera(mouse, camera);

    const intersects = raycaster.intersectObjects(imported, true);

    if (intersects.length > 0) {
        const selectedObject = intersects[0].object;
        selectedObject.clicked();
    }
}


function clearScene() {
    imported.forEach(model => {
        if (model.geometry) model.geometry.dispose();
        if (model.material) model.material.dispose();
        if (model.hasBoundingBox) model.removeBoundingBox();
        scene.remove(model);
    });

    renderer.render(scene, camera);
}

function clearAll() {
    clearScene();
    sliced = false;
    imported = [];
    layers = [];
    stlDataPointer = null;
    stlSize = 0;
    stlName = null;
    gcodeStr = null;

    scene.children.filter(child => child.userData.isGCodePath).forEach(child => {
        scene.remove(child);
        if (child.geometry) child.geometry.dispose();
        if (child.material) child.material.dispose();
    });

    exportGcodeButton.style.display = "none";
    loadSTLButton.value = "";
    exportGcodeButton.href = "";
    exportGcodeButton.download = "";
}


// -----------------------------------------------------------------------------


let scene, camera, renderer, controls, printBed, light;
let imported = [];
let layers = [];
let sliced = false;

init();

const loadSTLButton = document.getElementById("loadSTLButton");
const sliceButton = document.getElementById("sliceButton");
const exportGcodeButton = document.getElementById("exportGcodeButton");

export let slicer;

slicerModule().then((module) => {
    slicer = module;
    console.log("WASM loaded");
}).catch((err) => {
    console.error("Failed to load WASM:", err);
});

let stlDataPointer = null;
let stlSize = 0;
let stlName = null;
let gcodeStr = null;

async function loadSTL(event) {
    const file = event.dataTransfer ? event.dataTransfer.files[0] : event.target.files[0];
    if (!file || !slicer) return;
    stlName = file.name.replace(/\.[^/.]+$/, "");
    console.log("Loading STL:", stlName);

    const arrayBuffer = await file.arrayBuffer();
    const byteArray = new Uint8Array(arrayBuffer);
    stlSize = byteArray.length;

    if (stlDataPointer) {
        slicer._free(stlDataPointer);
    }

    stlDataPointer = slicer._malloc(stlSize);
    slicer.HEAPU8.set(byteArray, stlDataPointer); 

    const triangleCount = slicer.ccall("parseSTL", "number", ["number", "number"], [stlDataPointer, stlSize]);
    console.log("Triangle count:", triangleCount);
}

renderer.domElement.addEventListener("dragover", (event) => {
    event.preventDefault();
});

renderer.domElement.addEventListener("drop", async (event) => {
    event.preventDefault();
    clearAll();
    displaySTL(event);
    await loadSTL(event);
});

loadSTLButton.addEventListener("click", () => {
    clearAll();
});

loadSTLButton.addEventListener("change", async (event) => {
    displaySTL(event);
    await loadSTL(event);
});

sliceButton.addEventListener("click", () => {
    if (!slicer || !stlDataPointer || sliced) {
        return;
    }

    gcodeStr = slicer.getGcode(0.2); 

    exportGcodeButton
    const blob = new Blob([gcodeStr], { type: 'text/plain' });
    const url = URL.createObjectURL(blob);

    exportGcodeButton.href = url;
    exportGcodeButton.download = `${stlName}.gcode`;

    clearScene(); 
    sliced = true;

    exportGcodeButton.style.display = "block";
    renderGCode();
});

window.addEventListener("keydown", (event) => {
    if (event.key === "Delete" || event.key === "Backspace") {
        imported.forEach(model => {
            if (model.selected) {
                clearAll();
            }
        })
    }
});

function renderGCode() {
    if (!gcodeStr) return;

    const layers = parseGCodeLayers(gcodeStr);
    
    const gcodeGroup = new THREE.Group();
    gcodeGroup.userData.isGCodePath = true;
    scene.add(gcodeGroup);

    layers.forEach((layer) => {
        const layerGroup = new THREE.Group();
        layerGroup.userData.isGCodePath = true;
        layerGroup.position.y = layer.height * 0.2;
        
        layer.polygons.forEach(polygon => {
            const path = createPathFromPolygon(polygon);
            layerGroup.add(path);
        });

        gcodeGroup.add(layerGroup);
    });
}

function parseGCodeLayers(gcode) {
    const lines = gcode.split('\n');
    const layers = [];
    let currentLayer = null;
    let currentPolygon = null;
    let lastPosition = { x: 0, y: 0, z: 0 };
    let extruding = false;

    lines.forEach(line => {
        if (line.includes(';LAYER:')) {
            const layerHeightMatch = line.match(/;LAYER:(\d+)/);
            if (layerHeightMatch) {
                const layerHeight = parseFloat(layerHeightMatch[1]);
                if (currentLayer) layers.push(currentLayer);
                currentLayer = {
                    height: layerHeight,
                    polygons: []
                };
                currentPolygon = null;
            }
            return;
        }

        if (line.startsWith(';') || !(line.startsWith('G0') || line.startsWith('G1'))) {
            return;
        }

        const parts = line.split(' ');
        const command = {};
        parts.forEach(part => {
            if (part.startsWith('X')) command.x = parseFloat(part.substring(1));
            if (part.startsWith('Y')) command.y = parseFloat(part.substring(1));
            if (part.startsWith('Z')) command.z = parseFloat(part.substring(1));
            if (part.startsWith('E')) command.e = parseFloat(part.substring(1));
        });

        const newPosition = {
            x: command.x !== undefined ? command.x : lastPosition.x,
            y: command.y !== undefined ? command.y : lastPosition.y,
            z: command.z !== undefined ? command.z : lastPosition.z
        };

        const newExtruding = command.e !== undefined && command.e > 0;

        if (newExtruding && !extruding) {
            currentPolygon = {
                points: [new THREE.Vector3(lastPosition.x, 0, lastPosition.y)]
            };
        } else if (!newExtruding && extruding && currentPolygon) {
            if (currentPolygon.points.length > 2) {
                currentPolygon.points.push(currentPolygon.points[0].clone());
                currentLayer.polygons.push(currentPolygon);
            }
            currentPolygon = null;
        }

        if (newExtruding && currentPolygon) {
            currentPolygon.points.push(new THREE.Vector3(newPosition.x, 0, newPosition.y));
        }

        extruding = newExtruding;
        lastPosition = newPosition;
    });

    if (currentLayer) layers.push(currentLayer);

    return layers;
}

function createPathFromPolygon(polygon) {
    const material = new THREE.LineBasicMaterial({ 
        color: 0xff0000,
        linewidth: 1
    });

    const geometry = new THREE.BufferGeometry().setFromPoints(polygon.points);
    const line = new THREE.Line(geometry, material);
    line.userData.isGCodePath = true;
    
    return line;
}