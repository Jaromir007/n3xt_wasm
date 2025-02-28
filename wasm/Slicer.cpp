#include <vector>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <emscripten.h>
#include <cstdlib>
#include <algorithm>
#include <cfloat>

using namespace std;

extern "C" {

struct P2 {
    float x, y;
};
    
struct Vec3 {
    float x, y, z;
};

struct Triangle {
    Vec3 v1, v2, v3;
    float minZ, maxZ;

    Triangle(Vec3 a, Vec3 b, Vec3 c) : v1(a), v2(b), v3(c) {
        minZ = min({v1.z, v2.z, v3.z});
        maxZ = max({v1.z, v2.z, v3.z});
    }
};

vector<Triangle> triangleBuffer;
vector<P2> sliceResultBuffer;
float minZ, maxZ;

EMSCRIPTEN_KEEPALIVE
int parseSTL(const uint8_t* data, int length) {
    if (length < 84) return 0;
    
    uint32_t numTriangles;
    memcpy(&numTriangles, data + 80, sizeof(uint32_t));

    if (length < 84 + numTriangles * 50) return 0;

    triangleBuffer.clear();
    const uint8_t* ptr = data + 84;

    minZ = FLT_MAX;
    maxZ = -FLT_MAX;

    for (uint32_t i = 0; i < numTriangles; ++i) {
        Vec3 v1, v2, v3;
        memcpy(&v1, ptr + 12, sizeof(Vec3));
        memcpy(&v2, ptr + 24, sizeof(Vec3));
        memcpy(&v3, ptr + 36, sizeof(Vec3));

        Triangle tri(v1, v2, v3);
        minZ = min(minZ, tri.minZ);
        maxZ = max(maxZ, tri.maxZ);
        
        triangleBuffer.push_back(tri);
        ptr += 50;
    }

    sort(triangleBuffer.begin(), triangleBuffer.end(), [](const Triangle& a, const Triangle& b) {
        return a.minZ < b.minZ;
    });

    return triangleBuffer.size();
}

EMSCRIPTEN_KEEPALIVE
P2* slice(float layerHeight, int* totalPoints) {
    sliceResultBuffer.clear();
    if (triangleBuffer.empty()) return nullptr;

    for (float zp = minZ; zp <= maxZ; zp += layerHeight) {
        vector<P2> layerPoints;

        for (const auto& tri : triangleBuffer) {
            if (tri.minZ > zp) break;
            if (tri.maxZ < zp) continue; 

            Vec3 edges[3][2] = {{tri.v1, tri.v2}, {tri.v2, tri.v3}, {tri.v3, tri.v1}};
            for (auto& edge : edges) {
                Vec3 p1 = edge[0], p2 = edge[1];
                if ((p1.z < zp && p2.z > zp) || (p1.z > zp && p2.z < zp)) {
                    float t = (zp - p1.z) / (p2.z - p1.z);
                    layerPoints.push_back({
                        p1.x + t * (p2.x - p1.x),
                        p1.y + t * (p2.y - p1.y),
                    });
                }
            }
        }

        if (!layerPoints.empty()) {
            sliceResultBuffer.insert(sliceResultBuffer.end(), layerPoints.begin(), layerPoints.end());
            sliceResultBuffer.push_back({-9999, -9999}); // Layer separator
        }
    }

    *totalPoints = sliceResultBuffer.size();
    return sliceResultBuffer.data();
}
}
