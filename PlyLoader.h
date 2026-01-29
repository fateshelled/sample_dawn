#pragma once

#include <vector>
#include <string>
#include <cstdint>

struct Vertex {
    float x, y, z;
    float intensity;
};

class PlyLoader {
public:
    static bool Load(const std::string& filename, std::vector<Vertex>& outVertices);
};
