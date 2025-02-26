#include "ParseSTL.hpp"
#include "Slicer.hpp"
#include <iostream>
#include <chrono>
#include <sstream>
#include <fstream>

using namespace std; 
using namespace std::chrono; 

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "No file path provided" << endl;
        return 1;
    }

    string filename = argv[1];
    float layerHeight = 0.2f;

    auto start = high_resolution_clock::now(); 
    auto triangles = parseSTL(filename);
    if (triangles.empty()) return 1;

    auto layers = slice(triangles, layerHeight);
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);

    cout << "Done" << "\n" << "[TIMING] Slicing took: " << duration.count() << " ms" << "\n"; 

    stringstream json_out;
    json_out << "[\n";

    for (size_t i = 0; i < layers.size(); ++i) {
        if (i > 0) json_out << ",\n";
        json_out << "  [\n";

        for (size_t j = 0; j < layers[i].size(); ++j) {
            const auto& p = layers[i][j];
            if (j > 0) json_out << ",\n";
            json_out << "    [" << p.x << ", " << p.y << "]";
        }

        json_out << "\n  ]";
    }

    json_out << "\n]";

    ofstream out_file("../../out.json");
    if (!out_file) {
        cerr << "Error opening file for writing." << endl;
        return 1;
    }

    out_file << json_out.str();
    out_file.close();

    cout << "JSON saved to out.json" << endl;

    
    return 0;
}
