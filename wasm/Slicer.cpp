#include <vector>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <emscripten.h>
#include <cstdlib>

using namespace std;

extern "C" {

struct Vec3 {
    float x, y, z;
};

struct Triangle {
    Vec3 v1, v2, v3;
};

vector<Triangle> triangleBuffer;
vector<Vec3> sliceResultBuffer;

EMSCRIPTEN_KEEPALIVE
int parseSTL(const uint8_t* data, int length) {
    if (length < 84) return 0;
    
    uint32_t numTriangles;
    memcpy(&numTriangles, data + 80, sizeof(uint32_t));

    if (length < 84 + numTriangles * 50) return 0;

    triangleBuffer.clear();
    const uint8_t* ptr = data + 84;

    for (uint32_t i = 0; i < numTriangles; ++i) {
        Triangle tri;
        memcpy(&tri.v1, ptr + 12, sizeof(Vec3));
        memcpy(&tri.v2, ptr + 24, sizeof(Vec3));
        memcpy(&tri.v3, ptr + 36, sizeof(Vec3));

        triangleBuffer.push_back(tri);
        ptr += 50;
    }
    return triangleBuffer.size();
}

EMSCRIPTEN_KEEPALIVE
Vec3* slice(float layerHeight, int* numPoints) {
    sliceResultBuffer.clear();
    
    if (triangleBuffer.empty()) {
        *numPoints = 0;
        return nullptr;
    }

    float minZ = triangleBuffer[0].v1.z, maxZ = triangleBuffer[0].v1.z;
    for (const auto& tri : triangleBuffer) {
        minZ = min({minZ, tri.v1.z, tri.v2.z, tri.v3.z});
        maxZ = max({maxZ, tri.v1.z, tri.v2.z, tri.v3.z});
    }

    for (float zp = minZ; zp <= maxZ; zp += layerHeight) {
        for (const auto& tri : triangleBuffer) {
            Vec3 edges[3][2] = {{tri.v1, tri.v2}, {tri.v2, tri.v3}, {tri.v3, tri.v1}};
            for (auto& edge : edges) {
                Vec3 p1 = edge[0], p2 = edge[1];
                if ((p1.z < zp && p2.z > zp) || (p1.z > zp && p2.z < zp)) {
                    float t = (zp - p1.z) / (p2.z - p1.z);
                    sliceResultBuffer.push_back({
                        p1.x + t * (p2.x - p1.x),
                        p1.y + t * (p2.y - p1.y),
                        zp
                    });
                }
            }
        }
    }

    *numPoints = sliceResultBuffer.size();
    return sliceResultBuffer.data();
}

}
