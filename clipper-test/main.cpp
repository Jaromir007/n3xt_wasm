#include <iostream>
#include <iomanip>
#include <cstring>
#include <fstream>
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
PathsD sliced; 

int parseSTL(const uint8_t* data, int length) {
    if (length < 84) return 0; 

    uint32_t numTriangles; 
    memcpy(&numTriangles, data + 80, sizeof(uint32_t)); 

    if (length < 84 + numTriangles * 50) return 0; 

    triangles.clear(); 
    const uint8_t* ptr = data + 84; 

    for(uint32_t i = 0; i < numTriangles * 50; i++) {
        Vec3 v1, v2, v3; 
        memcpy(&v1, ptr + 12, sizeof(Vec3)); 
        memcpy(&v2, ptr + 24, sizeof(Vec3)); 
        memcpy(&v3, ptr + 32, sizeof(Vec3)); 

        triangles.emplace_back(v1, v2, v3); 
        ptr += 50; 
    }
    return triangles.size(); 
}

int slice(float layerHeight) {
    sliced.clear(); 

    if (triangles.empty())  {
        cout << "triangles are empty!"; 
        return 0; 
    };

    float minZ = triangles[0].minZ, maxZ = triangles[0].maxZ; 
    for (const auto& tri : triangles) {
        minZ = min(minZ, tri.minZ); 
        maxZ = max(maxZ, tri.maxZ); 
    }

    for (float zp = minZ; zp <= maxZ; zp += layerHeight) {
        PathsD layer; 

        for(const auto& tri : triangles) {
            if (zp < tri.minZ || zp > tri.maxZ) continue; 
            Vec3 edges[3][2] = {{tri.v1, tri.v2}, {tri.v2, tri.v3}, {tri.v3, tri.v1}}; 
            PathD path; 
            for (auto& edge : edges) {
                Vec3 p1 = edge[0], p2 = edge[1]; 
                if ((p1.z < zp && p2.z > zp) || (p1.z > zp && p2.z < zp)) {
                    float t = (zp - p1.z) / (p2.z - p1.z); 
                    
                    path.push_back({
                        p1.x + t * (p2.x - p1.x), 
                        p1.y + t * (p2.y - p1.y)
                    }); 
                }
            }
            if (path.size() >= 2) layer.push_back(path); 
        }
        
        if (!layer.empty()) {
            sliced.insert(sliced.end(), layer.begin(), layer.end()); 
        }
    }

    return 0; 
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "no arguments provided!"; 
        return 1; 
    }

    string filePath = argv[1]; 
    float layerHeight = (argc >= 3) ? stof(argv[2]) : 0.2f; 

    ifstream file(filePath, ios::binary | ios::ate);
    
    if (!file) {
        cout << "file not found!"; 
        return 1; 
    }

    streamsize fileSize = file.tellg(); 
    vector<uint8_t> buffer(fileSize); 
    file.seekg(0, ios::beg); 
    file.read(reinterpret_cast<char*>(buffer.data()), buffer.size()); 
    file.close(); 

    int trianglesSize = parseSTL(buffer.data(), buffer.size()); 
    slice(layerHeight); 

    cout << "slicing performed successfully, sliced " << trianglesSize / 50 << " triangles in total." << endl; 
    cout << "Total sliced paths: " << sliced.size() << endl;

    PathsD cleanContours;
    ClipperD clipper;
    clipper.AddSubject(sliced);
    clipper.Execute(ClipType::Union, FillRule::NonZero, cleanContours);
    
    vector<PathD> outerContours;
    vector<PathD> holes;
    
    for (const auto& path : cleanContours) {
        if (IsPositive(path)) { 
            outerContours.push_back(path); 
        } else { 
            holes.push_back(path); 
        }
    }
    
    cout << "Total outer contours: " << outerContours.size() << endl;
    cout << "Total holes: " << holes.size() << endl;

    int openPaths = 0;
    for (const auto& path : sliced) {
        if (path.size() < 3 || path.front() != path.back()) {  // Check if first and last point match
            openPaths++;
        }
    }
    cout << "Open paths: " << openPaths << " / " << sliced.size() << endl;

        
}
