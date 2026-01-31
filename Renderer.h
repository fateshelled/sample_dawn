#pragma once

#include <webgpu/webgpu_cpp.h>
#include <vector>
#include "PlyLoader.h"

struct GLFWwindow;

struct Uniforms {
    float scale;
    float padding[3]; // Padding to align to 16 bytes if necessary, though float is 4 bytes. 
                      // Uniform buffers usually require 16-byte alignment for structs in some backends/languages,
                      // but single float might be fine. Safe to pad or just align in shader.
                      // Let's stick to simple scalar if possible, but padding is safer for std140 layout compatibility if we expand.
                      // For a single float, simple is okay, but let's be explicit about the structure we bind.
};


class Renderer {
public:
    Renderer();
    ~Renderer();

    bool Initialize(GLFWwindow* window, const std::string& preferredDevice = "");
    void SetVertices(const std::vector<Vertex>& vertices);
    void Render();
    void Zoom(float delta);

private:
    void UpdateUniforms();

    wgpu::Instance instance;
    wgpu::Device device;
    wgpu::Queue queue;
    wgpu::Surface surface;
    wgpu::RenderPipeline pipeline;
    wgpu::Buffer vertexBuffer;
    uint32_t vertexCount = 0;
    wgpu::TextureFormat format = wgpu::TextureFormat::BGRA8Unorm;

    wgpu::Buffer uniformBuffer;
    wgpu::BindGroup bindGroup;
    wgpu::BindGroupLayout bindGroupLayout; // Store layout if needed, or just create it temporarily. Better to keep if we change pipeline.

    float zoomLevel = 1.0f;


    bool InitDevice(const std::string& preferredDevice);
    bool InitSurface(GLFWwindow* window);
    bool InitPipeline();
};
