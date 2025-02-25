#include <iostream>
#include <vector>
#include <fstream>
#include <cstdint>
#include <emscripten/bind.h>

struct Vec3 {
    float x, y, z;
};

struct Triangle {
    Vec3 v1, v2, v3;
};

std::vector<Triangle> parseSTL(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot open STL file.");
    }

    file.seekg(80);
    uint32_t numTriangles;
    file.read(reinterpret_cast<char*>(&numTriangles), sizeof(uint32_t));

    std::vector<Triangle> triangles(numTriangles);
    for (uint32_t i = 0; i < numTriangles; ++i) {
        Vec3 normal;
        file.read(reinterpret_cast<char*>(&normal), sizeof(Vec3)); 
        file.read(reinterpret_cast<char*>(&triangles[i].v1), sizeof(Vec3));
        file.read(reinterpret_cast<char*>(&triangles[i].v2), sizeof(Vec3));
        file.read(reinterpret_cast<char*>(&triangles[i].v3), sizeof(Vec3));
        file.seekg(2, std::ios::cur);
    }
    return triangles;
}

EMSCRIPTEN_BINDINGS(parser) {
    using namespace emscripten;

    value_object<Vec3>("Vec3")
        .field("x", &Vec3::x)
        .field("y", &Vec3::y)
        .field("z", &Vec3::z);

    value_object<Triangle>("Triangle")
        .field("v1", &Triangle::v1)
        .field("v2", &Triangle::v2)
        .field("v3", &Triangle::v3);

    register_vector<Triangle>("VTriangle");

    function("parseSTL", &parseSTL);
}
