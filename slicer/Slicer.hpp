#ifndef SLICER_HPP
#define SLICER_HPP

#include <vector>
#include "Triangle.hpp"

std::vector<std::vector<Vec3>> slice(const std::vector<Triangle>& triangles, float layerHeight);

#endif
