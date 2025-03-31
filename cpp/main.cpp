#include <iostream>
#include <iomanip>
#include <cstring>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "clipper2/clipper.h"
#include <emscripten.h>
#include <emscripten/bind.h>


using namespace std;
using namespace Clipper2Lib;

struct Vec3;
struct P2;
struct Triangle;
struct Edge;
typedef vector<Edge> Edges;
typedef vector<P2> Polygon;

struct Vec3 {
    float x, y, z; 

    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    bool operator==(const Vec3& v) const {
        return x == v.x && y == v.y && z == v.z;
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

vector<Triangle> triangles; 
vector<vector<Polygon>> sliced;

const double SCALE_FACTOR = 1000.0; 
const double NOZZLE_DIAMETER = 0.4;  
const double WALL_THICKNESS = NOZZLE_DIAMETER * 1.2; 
const int WALL_COUNT = 3;  

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

void cleanUpEdges(Edges& edges) {
    unordered_set<int> toRemove;

    for (size_t i = 0; i < edges.size(); ++i) {
        if(edges[i].v1 == edges[i].v2) {
            toRemove.insert(i);
            continue; 
        }
        for (size_t j = i + 1; j < edges.size(); ++j) {
            if (edges[i].directEqual(edges[j])) {
                toRemove.insert(i);
                toRemove.insert(j);
            }
        }
    }

    Edges cleanedEdges;
    for (size_t i = 0; i < edges.size(); ++i) {
        if (toRemove.find(i) == toRemove.end()) {
            cleanedEdges.push_back(edges[i]);
        }
    }

    edges = std::move(cleanedEdges);
}

Polygon simplifyPolygon(Polygon& polygon) {
    if (polygon.size() <= 3) return polygon;

    const float eps = 0.1f; 
    const float angleEps = 1.0f; 

    Polygon result;
    result.push_back(polygon[0]);

    for (size_t i = 1; i < polygon.size() - 1; i++) {
        const P2& prev = result.back();
        const P2& curr = polygon[i];
        const P2& next = polygon[i+1];

        float v1x = curr.x - prev.x;
        float v1y = curr.y - prev.y;
        float v2x = next.x - curr.x;
        float v2y = next.y - curr.y;

        float cross = v1x * v2y - v1y * v2x;
        float dot = v1x * v2x + v1y * v2y;
        float angle = atan2(fabs(cross), dot) * 180.0f / M_PI;

        if (fabs(angle) > angleEps || 
            fabs(cross) > eps) {
            result.push_back(curr);
        }
    }

    result.push_back(polygon.back());

    Polygon simplified;
    simplified.push_back(result[0]);
    for (size_t i = 1; i < result.size(); i++) {
        float dx = result[i].x - simplified.back().x;
        float dy = result[i].y - simplified.back().y;
        if (dx*dx + dy*dy > eps*eps) {
            simplified.push_back(result[i]);
        }
    }

    if (simplified.size() < 3) return polygon;

    return simplified;
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
            polygons.push_back(simplifyPolygon(polygon));
        }
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
    
    subject = SimplifyPaths(subject, 0.01 * SCALE_FACTOR);
    
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

string generateGCode(const vector<vector<Polygon>>& sliced, float layerHeight) {
    stringstream gcode;
    
    const float nozzleDiameter = 0.4f;
    const float filamentDiameter = 1.75f;
    const float extrusionMultiplier = 0.8f;
    const float firstLayerHeight = 0.2f;
    const float bedTemp = 60.0f;
    const float nozzleTemp = 215.0f;
    const float printSpeed = 60.0f;
    const float travelSpeed = 120.0f;
    const float firstLayerSpeed = 20.0f;
    
    const float perimWidth = 0.45f;
    const float firstLayerWidth = 0.42f;
    
    const float extrusionWidth = (layerHeight == firstLayerHeight) ? firstLayerWidth : perimWidth;
    const float layerArea = extrusionWidth * layerHeight;
    const float filamentArea = (filamentDiameter/2) * (filamentDiameter/2) * M_PI;
    const float extrusionFactor = layerArea / filamentArea * extrusionMultiplier;
    
    gcode << fixed << setprecision(3);
    
    gcode << "; Generated by N3XT Slicer\n";
    gcode << "; " << __DATE__ << " " << __TIME__ << "\n\n";

    gcode << "; Layer height: " << layerHeight << "\n";
    gcode << "; Nozzle diameter: " << nozzleDiameter << "\n";
    gcode << "; Filament diameter: " << filamentDiameter << "\n";
    gcode << "; Extrusion multiplier: " << extrusionMultiplier << "\n\n";
    
    gcode << "M73 P0 R128\n";
    gcode << "M73 Q0 S129\n";
    gcode << "M201 X1000 Y1000 Z1000 E5000 ; sets maximum accelerations, mm/sec^2\n";
    gcode << "M203 X200 Y200 Z12 E120 ; sets maximum feedrates, mm/sec\n";
    gcode << "M204 P1250 R1250 T1250 ; sets acceleration (P, T) and retract acceleration (R), mm/sec^2\n";
    gcode << "M205 X8.00 Y8.00 Z0.40 E2.50 ; sets the jerk limits, mm/sec\n";
    gcode << "M205 S0 T0 ; sets the minimum extruding and travel feed rate, mm/sec\n";
    gcode << "M107 ; disable fan\n";
    gcode << "M862.1 P" << nozzleDiameter << " ; nozzle diameter check\n";
    gcode << "M115 U3.8.1 ; tell printer latest fw version\n";
    
    gcode << "G90 ; use absolute coordinates\n";
    gcode << "M83 ; extruder relative mode\n";
    gcode << "M104 S" << nozzleTemp << " ; set extruder temp\n";
    gcode << "M140 S" << bedTemp << " ; set bed temp\n";
    gcode << "M190 S" << bedTemp << " ; wait for bed temp\n";
    gcode << "M109 S" << nozzleTemp << " ; wait for extruder temp\n";
    gcode << "G28 W ; home all without mesh bed level\n";
    gcode << "G80 ; mesh bed leveling\n";
    gcode << "G1 Y-3.0 F" << travelSpeed * 60 << " ; go outside print area\n";
    gcode << "G92 E0.0\n";
    gcode << "G1 X60.0 E9.0 F" << firstLayerSpeed * 60 << " ; intro line\n";
    gcode << "M73 Q0 S129\n";
    gcode << "M73 P0 R128\n";
    gcode << "G1 X100.0 E12.5 F" << firstLayerSpeed * 60 << " ; intro line\n";
    gcode << "G92 E0.0\n";
    gcode << "M221 S" << (extrusionMultiplier * 100) << " ; set flow multiplier\n";
    gcode << "G21 ; set units to millimeters\n";
    gcode << "G90 ; use absolute coordinates\n";
    gcode << "M83 ; use relative distances for extrusion\n";
    
    float currentZ = firstLayerHeight;
    bool firstLayer = true;
    
    for (size_t layerIdx = 0; layerIdx < sliced.size(); layerIdx++) {
        const auto& layer = sliced[layerIdx];
        
        gcode << "\n;LAYER:" << layerIdx << "\n";
        gcode << "G1 Z" << currentZ << " F" << printSpeed*60 << "\n";
        
        if (firstLayer) {
            gcode << "M107 ; fan off for first layer\n";
        } else if (layerIdx == 1) {
            gcode << "M106 S" << 50 << " ; fan 50% for second layer\n";
        } else if (layerIdx == 2) {
            gcode << "M106 S" << 100 << " ; full fan from third layer\n";
        }
        
        float lastX = 0, lastY = 0;
        bool firstPointInLayer = true;
        
        for (const auto& polygon : layer) {
            if (polygon.size() < 3) continue;
            
            float travelX = polygon[0].x/SCALE_FACTOR;
            float travelY = polygon[0].y/SCALE_FACTOR;
            
            gcode << "G0 X" << travelX 
                  << " Y" << travelY 
                  << " F" << travelSpeed*60 << "\n";
            
            gcode << "G92 E0 ; reset extruder\n";
            gcode << "G1 F" << (firstLayer ? firstLayerSpeed*60 : printSpeed*60) << "\n";
            
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
            
            lastX = polygon.back().x/SCALE_FACTOR;
            lastY = polygon.back().y/SCALE_FACTOR;
            firstPointInLayer = false;
        }
        
        firstLayer = false;
        currentZ += layerHeight;
    }
    
    gcode << "G4 ; wait\n";
    gcode << "M221 S100 ; reset flow\n";
    gcode << "M900 K0 ; reset LA\n";
    gcode << "M104 S0 ; turn off temperature\n";
    gcode << "M140 S0 ; turn off heatbed\n";
    gcode << "M107 ; turn off fan\n";
    gcode << "G1 Z" << currentZ + 5 << " F" << printSpeed*60 << " ; lift Z\n";
    gcode << "G1 X0 Y200 F3000 ; home X axis\n";
    gcode << "M84 ; disable motors\n";
    gcode << "M73 P100 R0\n";
    gcode << "M73 Q100 S0\n";
    
    return gcode.str();
}

EMSCRIPTEN_KEEPALIVE
int parseSTL(const uint8_t* stlPointer, uint32_t stlSize) {
    if (stlSize < 84) return 0; 
    
    uint32_t numTriangles;
    memcpy(&numTriangles, stlPointer + 80, sizeof(uint32_t));

    if (stlSize < 84 + numTriangles * 50) return 0;

    triangles.clear();
    const uint8_t* ptr = stlPointer + 84;

    for (size_t i = 0; i < numTriangles; i++) {
        Vec3 normal, v1, v2, v3;
        memcpy(&normal, ptr, sizeof(Vec3));
        memcpy(&v1, ptr + 12, sizeof(Vec3));
        memcpy(&v2, ptr + 24, sizeof(Vec3));
        memcpy(&v3, ptr + 36, sizeof(Vec3));

        triangles.emplace_back(v1, v2, v3, normal);
        ptr += 50;
    }

    return triangles.size();
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
        Edges edges; 

        for(const auto& tri : triangles) {
            if (zp < tri.minZ || zp > tri.maxZ) continue; 
            Vec3 triEdges[3][2] = {{tri.v1, tri.v2}, {tri.v2, tri.v3}, {tri.v3, tri.v1}}; 
            Vec3 normal = tri.normal;
            vector<P2> intersections; 
            for (auto& edge : triEdges) {
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
                    edges.push_back(Edge(intersections[0], intersections[1], normal, true));
                    edges.push_back(Edge(intersections[0], intersections[2], normal, true));
                    edges.push_back(Edge(intersections[1], intersections[2], normal, true));
                }
                else if(intersections.size() == 2){
                    edges.push_back(Edge(intersections[0], intersections[1], normal, true));
                }                
            }                               
        }
        if (!edges.empty()) {
            cleanUpEdges(edges); 
            vector<Polygon> layer = connectEdges(edges);
            vector<vector<Polygon>> walls = createWalls(layer);

            for (const auto& wall : walls) {
                layer.insert(layer.end(), wall.begin(), wall.end());
            }   
            sliced.push_back(layer);         
        }
    }

    return sliced.size(); 
}

string getGcode(float layerHeight) {
    slice(layerHeight); 
    return generateGCode(sliced, layerHeight);
}

EMSCRIPTEN_BINDINGS(SlicerModule) {

    emscripten::value_array<Vec3>("Vec3")
        .element(&Vec3::x)
        .element(&Vec3::y)
        .element(&Vec3::z);
        
    emscripten::value_array<P2>("P2")
        .element(&P2::x)
        .element(&P2::y);
        
    emscripten::register_vector<Vec3>("VectorVec3");
    emscripten::register_vector<P2>("VectorP2");
    emscripten::register_vector<Polygon>("VectorPolygon");
    emscripten::register_vector<vector<Polygon>>("VectorVectorPolygon");
    emscripten::register_vector<uint8_t>("VectorUint8");
    
    emscripten::function("parseSTL", &parseSTL, emscripten::allow_raw_pointers());
    emscripten::function("slice", &slice);
    emscripten::function("getGcode", &getGcode);
}