#ifndef SLICER_HPP
#define SLICER_HPP

#include <vector>
#include "Triangle.hpp"

using namespace std; 

vector<vector<Vec3>> slice(const vector<Triangle>& triangles, float layerHeight);

#endif
