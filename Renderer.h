#pragma once

#include <webgpu/webgpu_cpp.h>
#include <vector>
#include "PlyLoader.h"

struct GLFWwindow;

class Renderer {
public:
    Renderer();
    ~Renderer();

    bool Initialize(GLFWwindow* window, const std::string& preferredDevice = "");
    void SetVertices(const std::vector<Vertex>& vertices);
    void Render();

private:
    wgpu::Instance instance;
    wgpu::Device device;
    wgpu::Queue queue;
    wgpu::Surface surface;
    wgpu::RenderPipeline pipeline;
    wgpu::Buffer vertexBuffer;
    uint32_t vertexCount = 0;
    wgpu::TextureFormat format = wgpu::TextureFormat::BGRA8Unorm;

    bool InitDevice(const std::string& preferredDevice);
    bool InitSurface(GLFWwindow* window);
    bool InitPipeline();
};
