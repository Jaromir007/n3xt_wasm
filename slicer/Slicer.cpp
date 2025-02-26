#include "Slicer.hpp"
#include <algorithm>

using namespace std; 

vector<vector<Vec3>> slice(const vector<Triangle>& triangles, float layerHeight) {
    vector<vector<Vec3>> layers;
    float minZ = triangles[0].v1.z, maxZ = triangles[0].v1.z;

    for (const auto& tri : triangles) {
        minZ = min({minZ, tri.v1.z, tri.v2.z, tri.v3.z});
        maxZ = max({maxZ, tri.v1.z, tri.v2.z, tri.v3.z});
    }

    for (float zi = minZ; zi <= maxZ; zi += layerHeight) {
        vector<Vec3> layer;
        for (const auto& tri : triangles) {
            Vec3 edges[3][2] = {{tri.v1, tri.v2}, {tri.v2, tri.v3}, {tri.v3, tri.v1}};
            for (auto& edge : edges) {
                Vec3 p1 = edge[0], p2 = edge[1];
                if ((p1.z < zi && p2.z > zi) || (p1.z > zi && p2.z < zi)) {
                    float t = (zi - p1.z) / (p2.z - p1.z);
                    layer.push_back({
                        p1.x + t * (p2.x - p1.x),
                        p1.y + t * (p2.y - p1.y),
                        zi
                    });
                }
            }
        }
        if (!layer.empty()) layers.push_back(layer);
    }
    
    return layers;
}
