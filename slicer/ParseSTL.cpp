#include "ParseSTL.hpp"
#include <fstream>
#include <iostream>
#include <cstdint>

using namespace std; 

vector<Triangle> parseSTL(const string& filename) {
    ifstream file(filename, ios::binary);
    vector<Triangle> triangles;

    if (!file) {
        cerr << "Failed to open STL. \n";
        return triangles;
    }

    file.seekg(80); 
    uint32_t numTriangles;
    file.read(reinterpret_cast<char*>(&numTriangles), sizeof(numTriangles));
    
    for (uint32_t i = 0; i < numTriangles; ++i) {
        float normal[3];
        Vec3 v[3];
        file.read(reinterpret_cast<char*>(normal), sizeof(normal));
        file.read(reinterpret_cast<char*>(&v[0]), sizeof(Vec3));
        file.read(reinterpret_cast<char*>(&v[1]), sizeof(Vec3));
        file.read(reinterpret_cast<char*>(&v[2]), sizeof(Vec3));
        triangles.push_back({v[0], v[1], v[2]});
        file.ignore(2);
    }

    return triangles;
}
