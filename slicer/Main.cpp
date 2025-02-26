#include "ParseSTL.hpp"
#include "Slicer.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "No file path provided" << std::endl;
        return 1;
    }

    std::string filename = argv[1];
    float layerHeight = 0.2f;

    auto triangles = parseSTL(filename);
    if (triangles.empty()) return 1;

    auto layers = slice(triangles, layerHeight);
    
    for (size_t i = 0; i < layers.size(); ++i) {
        std::cout << "Layer " << i << " (Z = " << (i * layerHeight) << ")\n";
        for (const auto& p : layers[i]) {
            std::cout << "(" << p.x << ", " << p.y << ", " << p.z << ")\n";
        }
    }
    
    return 0;
}
