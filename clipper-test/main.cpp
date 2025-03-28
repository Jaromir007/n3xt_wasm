#include <iostream>
#include <iomanip>
#include <cstring>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "clipper2/clipper.h"

using namespace std;
using namespace Clipper2Lib;

struct Vec3 {
    float x, y, z; 

    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    bool operator==(const Vec3& v) const {
        return x == v.x && y == v.y && z == v.z;
    }

    friend ostream& operator<<(ostream& os, const Vec3& v) {
        os << v.x << ", " << v.y << ", " << v.z;
        return os;
    }
}; 

struct P2 {
    float x, y; 

    P2() : x(0), y(0) {}
    P2(float x, float y) : x(x), y(y) {}

    bool operator==(const P2& p) const {
        return x == p.x && y == p.y; 
    }

    bool operator!=(const P2& p) const {
        return !(*this == p);
    }

    friend ostream& operator<<(ostream& os, const P2& p) {
        os << p.x << ", " << p.y;
        return os;
    }

    P2 scale(const double& scale_factor) const {
        return P2(x * scale_factor, y * scale_factor);
    }

    struct Hash {
        size_t operator()(const P2& p) const {
            size_t h1 = hash<float>()(p.x);
            size_t h2 = hash<float>()(p.y);
            return h1 ^ (h2 << 1);
        }
    };
}; 

struct Triangle {
    Vec3 v1, v2, v3, normal;  
    float minZ, maxZ; 

    Triangle(const Vec3& v1, const Vec3& v2,const Vec3& v3, const Vec3& normal) 
        : v1(v1), v2(v2), v3(v3), normal(normal) {
            minZ = min({v1.z, v2.z, v3.z}); 
            maxZ = max({v1.z, v2.z, v3.z}); 
        }
}; 

struct Edge {
    P2 v1, v2;
    Vec3 normal;
    bool horisontal; 
    Edge(const P2& v1, const P2& v2, const Vec3& normal, bool sorted = false) : v1(v1), v2(v2), normal(normal) {
        if (normal.x == 0 && normal.y == 0) horisontal = true; 
        if (sorted) sortVertices();
    }

    bool operator==(const Edge& e) {
        return v1 == e.v1 && v2 == e.v2;
    }

    bool directEqual(const Edge& e) {
        float eps = 1;
        return abs(v1.x - e.v1.x) < eps && abs(v1.y - e.v1.y) < eps &&
               abs(v2.x - e.v2.x) < eps && abs(v2.y - e.v2.y) < eps &&
               normal == e.normal;
    }

    void sortVertices() {
        if (v1.x > v2.x || (v1.x == v2.x && v1.y > v2.y)) {
            swap(this->v1, this->v2);
        }
    }

    void swapVertices() {
        swap(this->v1, this->v2);
    }
}; 


typedef vector<Edge> Edges;
typedef vector<P2> Polygon; 

vector<Triangle> triangles; 
vector<vector<Polygon>> sliced;

const double SCALE_FACTOR = 1000.0; 
const double NOZZLE_DIAMETER = 0.4;  
const double WALL_THICKNESS = NOZZLE_DIAMETER * 1.2; 
const int WALL_COUNT = 3;  

int parseSTL(const uint8_t* data, int length) {
    if (length < 84) return 0; 

    uint32_t numTriangles; 
    memcpy(&numTriangles, data + 80, sizeof(uint32_t)); 

    if (length < 84 + numTriangles * 50) return 0; 

    triangles.clear(); 
    const uint8_t* ptr = data + 84; 

    for(int i = 0; i < numTriangles; i++) {
        Vec3 v1, v2, v3, normal; 
        memcpy(&normal, ptr, sizeof(Vec3)); 
        memcpy(&v1, ptr + 12, sizeof(Vec3)); 
        memcpy(&v2, ptr + 24, sizeof(Vec3)); 
        memcpy(&v3, ptr + 36, sizeof(Vec3)); 

        triangles.emplace_back(v1, v2, v3, normal); 
        ptr += 50; 
    }
    return triangles.size(); 
}

void cleanUpEdges(Edges& edges) {
    unordered_set<int> toRemove;

    for (int i = 0; i < edges.size(); ++i) {
        if(edges[i].v1 == edges[i].v2) {
            toRemove.insert(i);
            continue; 
        }
        for (int j = i + 1; j < edges.size(); ++j) {
            if (edges[i].directEqual(edges[j])) {
                toRemove.insert(i);
                toRemove.insert(j);
            }
        }
    }

    Edges cleanedEdges;
    for (int i = 0; i < edges.size(); ++i) {
        if (toRemove.find(i) == toRemove.end()) {
            cleanedEdges.push_back(edges[i]);
        }
    }

    edges = move(cleanedEdges);
}

vector<Polygon> connectEdges(Edges& layer) {
    vector<Polygon> polygons;
    if (layer.empty()) return polygons;

    auto pointsEqual = [](const P2& a, const P2& b) {
        const float eps = 1; 
        return abs(a.x - b.x) < eps && abs(a.y - b.y) < eps;
    };

    Edges remainingEdges = layer;

    while (!remainingEdges.empty()) {
        Polygon polygon;
        
        Edge currentEdge = remainingEdges.back();
        remainingEdges.pop_back();
        
        polygon.push_back(currentEdge.v1);
        P2 currentPoint = currentEdge.v2;
        polygon.push_back(currentPoint);

        bool foundNext = true;
        while (foundNext && !pointsEqual(currentPoint, polygon[0])) {
            foundNext = false;
            
            for (auto it = remainingEdges.begin(); it != remainingEdges.end(); ++it) {
                if (pointsEqual(it->v1, currentPoint)) {
                    currentEdge = *it;
                    remainingEdges.erase(it);
                    currentPoint = currentEdge.v2;
                    polygon.push_back(currentPoint);
                    foundNext = true;
                    break;
                }
                else if (pointsEqual(it->v2, currentPoint)) {
                    currentEdge = *it;
                    remainingEdges.erase(it);
                    currentPoint = currentEdge.v1;
                    polygon.push_back(currentPoint);
                    foundNext = true;
                    break;
                }
            }
        }

        if (polygon.size() >= 3 && pointsEqual(polygon.back(), polygon[0])) {
            polygons.push_back(polygon);
        }
    }

    return polygons;
}

int slice(float layerHeight) {
    sliced.clear(); 

    if (triangles.empty())  {
        cout << "[slice] Triangles are empty!"; 
        return 0; 
    };

    float minZ = triangles[0].minZ, maxZ = triangles[0].maxZ; 
    for (const auto& tri : triangles) {
        minZ = min(minZ, tri.minZ); 
        maxZ = max(maxZ, tri.maxZ); 
    }

    for (float zp = minZ; zp <= maxZ; zp += layerHeight) {
        Edges layer; 
        
        for(const auto& tri : triangles) {
            if (zp < tri.minZ || zp > tri.maxZ) continue; 
            Vec3 edges[3][2] = {{tri.v1, tri.v2}, {tri.v2, tri.v3}, {tri.v3, tri.v1}}; 
            Vec3 normal = tri.normal;
            vector<P2> intersections; 
            for (auto& edge : edges) {
                Vec3 v1 = edge[0], v2 = edge[1]; 
                double x; 
                double y; 
                if ((v1.z <= zp && v2.z >= zp) || (v1.z >= zp && v2.z <= zp)) {
                    float t = (zp - v1.z) / (v2.z - v1.z); 
                    t = max(0.0f, min(1.0f, t)); 
                    x = v1.x + t * (v2.x - v1.x); 
                    y = v1.y + t * (v2.y - v1.y); 
                    intersections.push_back(P2(x * SCALE_FACTOR, round(y * SCALE_FACTOR)));
                }
            }
            if (!intersections.empty()) {
                if(intersections.size() == 3) {
                    layer.push_back(Edge(intersections[0], intersections[1], normal, true));
                    layer.push_back(Edge(intersections[0], intersections[2], normal, true));
                    layer.push_back(Edge(intersections[1], intersections[2], normal, true));
                }
                else if(intersections.size() == 2){
                    layer.push_back(Edge(intersections[0], intersections[1], normal, true));
                }                
            }                               
        }
        if (!layer.empty()) {
            cleanUpEdges(layer); 
            sliced.emplace_back(connectEdges(layer)); 
        }
    }

    return 0; 
}

PathsD polygonsToPaths(const vector<Polygon>& polygons) {
    PathsD paths;
    for (const auto& poly : polygons) {
        PathD path;
        for (const auto& p : poly) {
            path.emplace_back(p.x, p.y);
        }
        paths.push_back(path);
    }
    return paths;
}

vector<Polygon> pathsToPolygons(const PathsD& paths) {
    vector<Polygon> polygons;
    for (const auto& path : paths) {
        Polygon poly;
        for (const auto& p : path) {
            poly.emplace_back(p.x, p.y);
        }
        polygons.push_back(poly);
    }
    return polygons;
}

vector<Polygon> offsetPolygons(const vector<Polygon>& polygons, double delta) {
    Paths64 subject;
    for (const auto& poly : polygons) {
        Path64 path;
        for (const auto& p : poly) {
            path.emplace_back(
                static_cast<int64_t>(p.x),
                static_cast<int64_t>(p.y)
            );
        }
        subject.push_back(path);
    }
    
    subject = SimplifyPaths(subject, 0.1 * SCALE_FACTOR);
    
    ClipperOffset co;
    co.AddPaths(subject, JoinType::Round, EndType::Polygon);
    Paths64 solution;
    co.Execute(-delta * SCALE_FACTOR, solution); 
    
    vector<Polygon> result;
    for (const auto& path : solution) {
        Polygon poly;
        for (const auto& p : path) {
            poly.emplace_back(
                static_cast<double>(p.x),
                static_cast<double>(p.y)
            );
        }
        result.push_back(poly);
    }
    
    return result;
}

vector<vector<Polygon>> createWalls(const vector<Polygon>& polygons) {
    vector<vector<Polygon>> walls;
    vector<Polygon> current = polygons;
    
    for (int i = 0; i < WALL_COUNT; ++i) {
        if (current.empty()) break;
        
        walls.push_back(current);
        current = offsetPolygons(current, WALL_THICKNESS);
    }
    
    return walls;
}

const double INFILL_SPACING = 4.0; 
const double INFILL_ANGLE = 45.0;  

// vector<Polygon> generateGridInfill(const vector<Polygon>& polygons) {
//     if (polygons.empty()) {
//         return {};
//     }

//     return pathsToPolygons(solution);
// }

void processLayers() {
    for (auto& layer : sliced) {
        vector<vector<Polygon>> walls = createWalls(layer);
        
        layer.clear();
        
        for (const auto& wall : walls) {
            layer.insert(layer.end(), wall.begin(), wall.end());
        }
        
        // if (!walls.empty() && !walls.back().empty()) {
        //     vector<Polygon> infill = generateGridInfill(walls.back());
        //     layer.insert(layer.end(), infill.begin(), infill.end());
        // }
    }
}

#include <sstream>
#include <iomanip>

ostringstream generateGCode(const vector<vector<Polygon>>& sliced, float layerHeight) {
    ostringstream gcode;
    
    const float nozzleDiameter = 0.4f;
    const float filamentDiameter = 1.75f;
    const float extrusionMultiplier = 0.9f;
    const float firstLayerHeight = 0.2f;
    const float bedTemp = 60.0f;
    const float nozzleTemp = 210.0f;
    const float printSpeed = 60.0f;
    const float travelSpeed = 120.0f;
    const float firstLayerSpeed = 20.0f;
    
    const float extrusionWidth = nozzleDiameter * 1.2f;
    const float layerArea = extrusionWidth * layerHeight;
    const float filamentArea = (filamentDiameter/2) * (filamentDiameter/2) * M_PI;
    const float extrusionFactor = layerArea / filamentArea * extrusionMultiplier;
    
    gcode << fixed << setprecision(3);
    
    gcode << "; Generated by N3XT Slicer\n";
    gcode << "; Layer height: " << layerHeight << "\n";
    gcode << "; Nozzle diameter: " << nozzleDiameter << "\n\n";
    
    gcode << "G21 ; Set units to millimeters\n";
    gcode << "G90 ; Use absolute positioning\n";
    gcode << "M83 ; Use relative distances for extrusion\n";
    gcode << "M104 S" << nozzleTemp << " ; Set nozzle temperature\n";
    gcode << "M140 S" << bedTemp << " ; Set bed temperature\n";
    gcode << "M190 S" << bedTemp << " ; Wait for bed temperature\n";
    gcode << "M109 S" << nozzleTemp << " ; Wait for nozzle temperature\n\n";
    
    gcode << "G28 ; Home all axes\n";
    gcode << "G1 Z" << firstLayerHeight << " F" << firstLayerSpeed*60 << "\n";
    gcode << "G92 E0 ; Reset extruder position\n";
    
    float currentZ = firstLayerHeight;
    bool firstLayer = true;
    
    for (size_t layerIdx = 0; layerIdx < sliced.size(); layerIdx++) {
        const auto& layer = sliced[layerIdx];
        
        gcode << "\n; LAYER:" << layerIdx << "\n";
        gcode << "G1 Z" << currentZ << " F" << printSpeed*60 << "\n";
        
        for (const auto& polygon : layer) {
            if (polygon.size() < 3) continue;
            
            gcode << "G0 X" << polygon[0].x/SCALE_FACTOR 
                  << " Y" << polygon[0].y/SCALE_FACTOR 
                  << " F" << travelSpeed*60 << "\n";
            
            gcode << "G92 E0\n";
            gcode << "G1 F" << (firstLayer ? firstLayerSpeed*60 : printSpeed*60) << "\n";
            
            float perimeterLength = 0.0f;
            for (size_t i = 1; i <= polygon.size(); i++) {
                size_t next = i % polygon.size();
                float dx = (polygon[next].x - polygon[i-1].x)/SCALE_FACTOR;
                float dy = (polygon[next].y - polygon[i-1].y)/SCALE_FACTOR;
                perimeterLength += sqrt(dx*dx + dy*dy);
            }
            
            for (size_t i = 1; i <= polygon.size(); i++) {
                size_t next = i % polygon.size();
                float dx = (polygon[next].x - polygon[i-1].x)/SCALE_FACTOR;
                float dy = (polygon[next].y - polygon[i-1].y)/SCALE_FACTOR;
                float segmentLength = sqrt(dx*dx + dy*dy);
                float extrusion = segmentLength * extrusionFactor;
                
                gcode << "G1 X" << polygon[next].x/SCALE_FACTOR 
                      << " Y" << polygon[next].y/SCALE_FACTOR
                      << " E" << extrusion << "\n";
            }
        }
        
        firstLayer = false;
        currentZ += layerHeight;
    }
    
    gcode << "\n; End G-code\n";
    gcode << "G1 Z" << currentZ + 5 << " F" << printSpeed*60 << "\n";
    gcode << "G1 X0 Y0 F" << travelSpeed*60 << "\n";
    gcode << "M104 S0\n";
    gcode << "M140 S0\n";
    gcode << "M107\n";
    gcode << "M84\n";
    
    return gcode;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "[main] No arguments provided!"; 
        return 1; 
    }

    string filePath = argv[1]; 
    float layerHeight = (argc >= 3) ? stof(argv[2]) : 0.2f; 

    ifstream file(filePath, ios::binary | ios::ate);
    
    if (!file) {
        cout << "[main] File not found!" << endl; 
        return 1; 
    }

    streamsize fileSize = file.tellg(); 
    vector<uint8_t> buffer(fileSize); 
    file.seekg(0, ios::beg); 
    file.read(reinterpret_cast<char*>(buffer.data()), buffer.size()); 
    file.close(); 

    int trianglesSize = parseSTL(buffer.data(), buffer.size()); 
    slice(layerHeight); 
    processLayers();

    cout << "slicing performed successfully, sliced " << trianglesSize << " triangles in total." << endl; 
    cout << "Total layers: " << sliced.size() << endl; 

    ostringstream gcode = generateGCode(sliced, layerHeight);
    ostringstream json; 

    json << "[" << endl;

    for (size_t i = 0; i < sliced.size(); ++i) {
        json << "   [" << endl;
        auto polygons = sliced[i];
        
        for (size_t j = 0; j < polygons.size(); ++j) {
            const auto& polygon = polygons[j];
            json << "       [" << endl;
            
            for (size_t k = 0; k < polygon.size(); ++k) {
                const auto& point = polygon[k];
                json << "        [" << point.x / SCALE_FACTOR << "," << point.y / SCALE_FACTOR << "]";
                if (k < polygon.size() - 1) json << "," << endl;
            }
            
            json << endl << "       ]";
            if (j < polygons.size() - 1) json << "," << endl;
        }
        
        json << endl << "   ]";
        if (i < sliced.size() - 1) json << "," << endl;
    }

    json << "]";
   
    ofstream outFile("out.json"); 
    if(outFile.is_open()) {
        outFile << json.str(); 
        outFile.close(); 
        cout << "saved to JSON" << endl;
    } else {
        cerr << "failed to open the file for writing" << endl; 
    }

    ofstream gcodeFile("out.gcode");
    if (gcodeFile.is_open()) {
        gcodeFile << gcode.str();
        gcodeFile.close();
        cout << "G-code saved to output.gcode" << endl;
    } else {
        cerr << "Failed to save G-code file" << endl;
    }

    return 0; 
}
