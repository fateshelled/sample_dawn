#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include "PlyLoader.h"
#include "Renderer.h"

int main(int argc, char** argv) {
    std::string filename;
    std::string preferredDevice;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--device" && i + 1 < argc) {
            preferredDevice = argv[++i];
        } else if (filename.empty()) {
            filename = arg;
        }
    }

    if (filename.empty()) {
        std::cerr << "Usage: " << argv[0] << " <ply_file> [--device <device_name_substring>]" << std::endl;
        return 1;
    }
    std::vector<Vertex> vertices;
    if (!PlyLoader::Load(filename, vertices)) {
        return 1;
    }
    std::cout << "Successfully loaded " << vertices.size() << " vertices from " << filename << std::endl;

    if (!glfwInit()) {
        return 1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Dawn PLY Viewer", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return 1;
    }

    Renderer renderer;
    if (!renderer.Initialize(window, preferredDevice)) {
        return 1;
    }

    renderer.SetVertices(vertices);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        renderer.Render();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
