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
}; 

struct Triangle {
    Vec3 v1, v2, v3; 
    float minZ, maxZ; 

    Triangle(const Vec3& v1, const Vec3& v2,const Vec3& v3) 
        : v1(v1), v2(v2), v3(v3) {
            minZ = min({v1.z, v2.z, v3.z}); 
            maxZ = max({v1.z, v2.z, v3.z}); 
        }
}; 

vector<Triangle> triangles; 
vector<Paths64> sliced; 

const double SCALE_FACTOR = 10000.0;

void sortEdge(Path64& edge) {
    if (edge.size() == 2) {
        if (edge[0].x > edge[1].x || (edge[0].x == edge[1].x && edge[0].y > edge[1].y)) {
            swap(edge[0], edge[1]);
        }
        return;
    }

    if (edge.size() == 3) {
        cout << "3 point edge " << edge << endl; 
        sort(edge.begin(), edge.end(), [](const Point64& a, const Point64& b) {
            return (a.x < b.x) || (a.x == b.x && a.y < b.y);
        });
        edge.erase(edge.begin() + 1);
        cout << "edge sorted " << edge << endl; 
    }
}

void sortLayer(Paths64& layer) {
    for (auto& edge : layer) {
        sortEdge(edge);
    }

    sort(layer.begin(), layer.end(), [](const Path64& a, const Path64& b) {
        return (a[0].x < b[0].x) || (a[0].x == b[0].x && a[0].y < b[0].y) ||
               (a[0].x == b[0].x && a[0].y == b[0].y && a[1].x < b[1].x) ||
               (a[0].x == b[0].x && a[0].y == b[0].y && a[1].x == b[1].x && a[1].y < b[1].y);
    });

    layer.erase(unique(layer.begin(), layer.end()), layer.end());
}

struct Point64Hash {
    size_t operator()(const Point64& p) const {
        return hash<int64_t>()(p.x) ^ (hash<int64_t>()(p.y) << 1);
    }
};

void connectEdges(Paths64& layer) {
    if (layer.empty()) return;

    unordered_map<Point64, vector<Point64>, Point64Hash> edgeMap;
    unordered_set<pair<Point64, Point64>, hash<pair<Point64, Point64>>> visitedEdges;

    // Insert edges bidirectionally
    for (const auto& edge : layer) {
        if (edge.size() != 2) continue; // Ensure it's a valid edge
        edgeMap[edge[0]].push_back(edge[1]);
        edgeMap[edge[1]].push_back(edge[0]);
    }

    Paths64 polygons;
    unordered_set<Point64, Point64Hash> visitedPoints;

    // Iterate over all edges
    while (!edgeMap.empty()) {
        Path64 polygon;
        auto it = edgeMap.begin();
        Point64 start = it->first;
        Point64 current = start;
        polygon.push_back(start);

        while (true) {
            if (edgeMap[current].empty()) break;

            Point64 next = edgeMap[current].back();
            edgeMap[current].pop_back();
            auto& nextEdges = edgeMap[next];

            // Remove reference to the current point from the next point's list
            nextEdges.erase(remove(nextEdges.begin(), nextEdges.end(), current), nextEdges.end());

            // Remove the entry if no more connections exist
            if (nextEdges.empty()) edgeMap.erase(next);

            polygon.push_back(next);
            current = next;

            // Close the loop
            if (current == start) {
                polygons.push_back(polygon);
                break;
            }
        }

        // If the polygon is incomplete, skip it
        if (polygon.size() < 3 || polygon.front() != polygon.back()) {
            continue;
        }

        edgeMap.erase(start);
    }

    layer = polygons;
}



int parseSTL(const uint8_t* data, int length) {
    if (length < 84) return 0; 

    uint32_t numTriangles; 
    memcpy(&numTriangles, data + 80, sizeof(uint32_t)); 

    if (length < 84 + numTriangles * 50) return 0; 

    triangles.clear(); 
    const uint8_t* ptr = data + 84; 

    for(int i = 0; i < numTriangles; i++) {
        Vec3 v1, v2, v3; 
        memcpy(&v1, ptr + 12, sizeof(Vec3)); 
        memcpy(&v2, ptr + 24, sizeof(Vec3)); 
        memcpy(&v3, ptr + 36, sizeof(Vec3)); 

        triangles.emplace_back(v1, v2, v3); 
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
        Paths64 layer; 
        
        for(const auto& tri : triangles) {
            if (zp <= tri.minZ || zp >= tri.maxZ) continue; 
            Vec3 edges[3][2] = {{tri.v1, tri.v2}, {tri.v2, tri.v3}, {tri.v3, tri.v1}}; 
            Path64 intersections; 
            for (auto& edge : edges) {
                Vec3 v1 = edge[0], v2 = edge[1]; 
                double x; 
                double y; 
                if ((v1.z <= zp && v2.z >= zp) || (v1.z >= zp && v2.z <= zp)) {
                    float t = (zp - v1.z) / (v2.z - v1.z); 
                    t = max(0.0f, min(1.0f, t)); 
                    x = v1.x + t * (v2.x - v1.x); 
                    y = v1.y + t * (v2.y - v1.y); 
                    intersections.push_back(Point64(x * SCALE_FACTOR, round(y * SCALE_FACTOR)));
                }
            }
            if (!intersections.empty()) {
                if (intersections.size() == 3) {
                    layer.push_back(Path64({intersections[0], intersections[1]}));
                    layer.push_back(Path64({intersections[1], intersections[2]}));
                    layer.push_back(Path64({intersections[2], intersections[0]}));
                } else {
                    layer.push_back(intersections);
                }
            }                               
        }
        if (!layer.empty()) {
            sortLayer(layer);
            connectEdges(layer); 
            sliced.push_back(layer);
        }
    }

    return 0; 
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

    cout << "slicing performed successfully, sliced " << trianglesSize << " triangles in total." << endl; 
    cout << "Total layers: " << sliced.size() << endl; 

    ostringstream json; 
    
    json << "[" << endl; 
    for(int i = 0; i < sliced.size(); i++) {
        json << "   [" << endl; 
        for(int j = 0; j < sliced[i].size(); j++) {
            json << "       [" << endl; 
            for(int k = 0; k < sliced[i][j].size(); k++) {
                json << "           [" << sliced[i][j][k].x / SCALE_FACTOR << ", " << sliced[i][j][k].y / SCALE_FACTOR << "]"; 
                if(k < sliced[i][j].size() -1) json << ", " << endl; 
            }
            json << endl << "       ]"; 
            if(j < sliced[i].size() -1) json << ", " << endl; 
        }
        json << endl << "   ]"; 
        if(i < sliced.size() -1) json << ", " << endl; 
    }
    json << endl << "]"; 

    ofstream outFile("out.json"); 
    if(outFile.is_open()) {
        outFile << json.str(); 
        outFile.close(); 
        cout << "saved to JSON" << endl;
    } else {
        cerr << "failed to open the file for writing" << endl; 
    }

    return 0; 
}
