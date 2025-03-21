#include <vector>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <emscripten.h>
#include <cstdlib>
#include "Clipper2Lib/include/clipper2/clipper.h"

using namespace std;
using namespace Clipper2Lib;

extern "C" {

struct Vec3 {
    float x, y, z;
};

struct Triangle {
    Vec3 v1, v2, v3;
    float minZ, maxZ;

    Triangle(const Vec3& v1, const Vec3& v2, const Vec3& v3)
        : v1(v1), v2(v2), v3(v3) {
        minZ = min({v1.z, v2.z, v3.z});
        maxZ = max({v1.z, v2.z, v3.z});
    }
};

vector<Triangle> triangles;
PathsD sliced;

EMSCRIPTEN_KEEPALIVE
int parseSTL(const uint8_t* data, int length) {
    if (length < 84) return 0;
    
    uint32_t numTriangles;
    memcpy(&numTriangles, data + 80, sizeof(uint32_t));

    if (length < 84 + numTriangles * 50) return 0;

    triangles.clear();
    const uint8_t* ptr = data + 84;

    for (uint32_t i = 0; i < numTriangles; ++i) {
        Vec3 v1, v2, v3;
        memcpy(&v1, ptr + 12, sizeof(Vec3));
        memcpy(&v2, ptr + 24, sizeof(Vec3));
        memcpy(&v3, ptr + 36, sizeof(Vec3));

        triangles.emplace_back(v1, v2, v3);
        ptr += 50;
    }
    return triangles.size();
}

EMSCRIPTEN_KEEPALIVE
PointD* slice(float layerHeight, int* totalPoints) {
    contours.clear();

    if (triangles.empty()) return nullptr;

    float minZ = triangles[0].minZ, maxZ = triangles[0].maxZ;
    for (const auto& tri : triangles) {
        minZ = min(minZ, tri.minZ);
        maxZ = max(maxZ, tri.maxZ);
    }

    for (float zp = minZ; zp <= maxZ; zp += layerHeight) {
        pathsD layer; 

        for (const auto& tri : triangles) {
            if (zp < tri.minZ || zp > tri.maxZ) continue;

            Vec3 edges[3][2] = {{tri.v1, tri.v2}, {tri.v2, tri.v3}, {tri.v3, tri.v1}};
            PathD path; 
            for (auto& edge : edges) {
                Vec3 p1 = edge[0], p2 = edge[1];
                if ((p1.z < zp && p2.z > zp) || (p1.z > zp && p2.z < zp)) {
                    float t = (zp - p1.z) / (p2.z - p1.z);
                    path.push_back({
                        p1.x + t * (p2.x - p1.x),
                        p1.y + t * (p2.y - p1.y),
                    });
                }
            }
            if (path.size() >= 2) layer.push_back(path); 
        }

        if (!layer.empty()) {
            PathsD contours = Union(layer, FillRule::NonZero); 
            sliced.insert(sliced.end(), contours.begin(), contours.end())

        }
    }
    return sliced.data();
}
}