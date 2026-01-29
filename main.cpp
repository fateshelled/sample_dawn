#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include "PlyLoader.h"
#include "Renderer.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <ply_file>" << std::endl;
        return 1;
    }

    std::string filename = argv[1];
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
    if (!renderer.Initialize(window)) {
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
