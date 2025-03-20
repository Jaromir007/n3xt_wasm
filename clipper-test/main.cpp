#include <iostream>
#include <iomanip>
#include <cstring>
#include <fstream>
#include <sstream>
#include <unordered_map>
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

const double SCALE_FACTOR = 1000000.0; // 1e6 = 1 micrometer

struct Point64Hash {
    size_t operator()(const Point64& p) const {
        return hash<int64_t>()(p.x) ^ (hash<int64_t>()(p.y) << 1);
    }
};

Paths64 connectEdges(const Paths64& edges) {
    unordered_map<Point64, Point64, Point64Hash> edgeMap; 

    return edges;
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
                if ((v1.z < zp && v2.z > zp) || (v1.z > zp && v2.z < zp)) {
                    float t = (zp - v1.z) / (v2.z - v1.z); 
                    x = v1.x + t * (v2.x - v1.x); 
                    y = v1.y + t * (v2.y - v1.y); 
                }

                Point64 p = {round(x * SCALE_FACTOR), round(y * SCALE_FACTOR)}; 
                if (intersections.empty() || intersections.back() != p) {
                    intersections.push_back(p);    
                }
            }
            if(!intersections.empty()) {
                layer.push_back(intersections); 
            }
        }
        if (!layer.empty()) {
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
        cout << "[main] File not found!"; 
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
    json << "]"; 

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
