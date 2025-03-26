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
    Vec3 v1, v2, v3, normal;  
    float minZ, maxZ; 

    Triangle(const Vec3& v1, const Vec3& v2,const Vec3& v3, const Vec3& normal) 
        : v1(v1), v2(v2), v3(v3), normal(normal) {
            minZ = min({v1.z, v2.z, v3.z}); 
            maxZ = max({v1.z, v2.z, v3.z}); 
        }
}; 

struct Edge {
    Point64 v1, v2;
    Vec3 normal;
    Edge(const Point64& v1, const Point64& v2, const Vec3& normal) : v1(v1), v2(v2), normal(normal) {
        if (v1.x > v2.x || (v1.x == v2.x && v1.y > v2.y)) {
            swap(this->v1, this->v2);
        }
    }
}; 


typedef vector<Edge> Edges;

vector<Triangle> triangles; 
vector<Edges> sliced; 

const double SCALE_FACTOR = 10000.0;

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
                if(intersections.size() == 3) {
                    layer.push_back(Edge(intersections[0], intersections[1], normal));
                    layer.push_back(Edge(intersections[0], intersections[2], normal));
                    layer.push_back(Edge(intersections[1], intersections[2], normal));
                }
                else if(intersections.size() == 2){
                    layer.push_back(Edge(intersections[0], intersections[1], normal));
                }                
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
        auto& layer = sliced[i]; 
        json << "   [" << endl; 
        for(int j = 0; j < layer.size(); j++) {
            auto& edge = layer[j];
            json << "       [" << endl; 
            json << "           [" << edge.v1 << "], " << endl; 
            json << "           [" << edge.v2 << "], " << endl; 
            json << "           [" << edge.normal.x << "," << edge.normal.y << "," << edge.normal.z << "] " << endl;
            json << endl << "       ]"; 
            if(j < layer.size() - 1) json << "," << endl; 
        }
        json << endl << "   ]"; 
        if(i < sliced.size() - 1) json << "," << endl; 
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
