#include "ToJSON.hpp"

using namespace std; 

int write_to_json(const vector<vector<Vec3>>& layers) {
    
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
