#include "ParseSTL.hpp"
#include "Slicer.hpp"
#include "ToJSON.hpp"
#include <iostream>
#include <chrono>

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

    write_to_json(layers);

    return 0;
}
