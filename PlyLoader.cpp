#include "PlyLoader.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

bool PlyLoader::Load(const std::string& filename, std::vector<Vertex>& outVertices) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open PLY file: " << filename << std::endl;
        return false;
    }

    std::string line;
    uint32_t vertexCount = 0;
    bool binary = false;

    // Read header
    while (std::getline(file, line)) {
        if (line.find("format binary_little_endian 1.0") != std::string::npos) {
            binary = true;
        } else if (line.find("element vertex") != std::string::npos) {
            std::stringstream ss(line);
            std::string temp;
            ss >> temp >> temp >> vertexCount;
        } else if (line.find("end_header") != std::string::npos) {
            break;
        }
    }

    if (!binary) {
        std::cerr << "Only binary_little_endian 1.0 is supported" << std::endl;
        return false;
    }

    if (vertexCount == 0) {
        std::cerr << "No vertices found" << std::endl;
        return false;
    }

    outVertices.resize(vertexCount);
    file.read(reinterpret_cast<char*>(outVertices.data()), vertexCount * sizeof(Vertex));

    if (!file) {
        std::cerr << "Failed to read binary data" << std::endl;
        return false;
    }

    return true;
}
