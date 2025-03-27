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

    P2(float x, float y) : x(x), y(y) {}

    bool operator==(const P2& p) const {
        return x == p.x && y == p.y; 
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
    Edge(const P2& v1, const P2& v2, const Vec3& normal) : v1(v1), v2(v2), normal(normal) {
        if (v1.x > v2.x || (v1.x == v2.x && v1.y > v2.y)) {
            swap(this->v1, this->v2);
        }
        if(abs(normal.x) == 0 && abs(normal.y) == 0) horisontal = true; 
    }

    bool operator==(const Edge& e) {
        return v1 == e.v1 && v2 == e.v2;
    }

    bool directEqual(const Edge& e) {
        float error = 1e-4;
        return abs(v1.x - e.v1.x) < error && abs(v1.y - e.v1.y) < error &&
               abs(v2.x - e.v2.x) < error && abs(v2.y - e.v2.y) < error &&
               normal == e.normal;
    }
}; 


typedef vector<Edge> Edges;
typedef vector<P2> Polygon; 

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

void cleanUpEdges(Edges& edges) {
    unordered_set<int> toRemove;

    for (int i = 0; i < edges.size(); ++i) {
        if(edges[i].v1 == edges[i].v2) {
            toRemove.insert(i);
            continue; 
        }
        if (!edges[i].horisontal) continue;
        for (int j = i + 1; j < edges.size(); ++j) {
            if (!edges[j].horisontal) continue;
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

void removeDuplicateEdges(Edges& edges) {
    return; 
}

void connectEdges(Edges& edges) {
    return;
}



vector<Polygon> formPolygons(Edges& edges) {
    vector<Polygon> polygons; 
    
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
            cleanUpEdges(layer); 
            removeDuplicateEdges(layer);
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

        for (size_t i = 0; i < sliced.size(); ++i) {
            json << "   [" << endl;
            const auto& layer = sliced[i];
            for (size_t j = 0; j < layer.size(); ++j) {
                const auto& edge = layer[j];
                json << "       [" << endl << "        [" << edge.v1.x / SCALE_FACTOR << "," << edge.v1.y / SCALE_FACTOR << "]," << endl << "        [" 
                     << edge.v2.x / SCALE_FACTOR << "," << edge.v2.y / SCALE_FACTOR << "]" << endl << "       ]";
                if (j < layer.size() - 1) json << "," << endl;
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

    return 0; 
}
