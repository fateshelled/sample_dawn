#pragma once

#include <webgpu/webgpu_cpp.h>
#include <vector>
#include "PlyLoader.h"

struct GLFWwindow;

struct Uniforms {
    float mvp[16];
};


class Renderer {
public:
    Renderer();
    ~Renderer();

    bool Initialize(GLFWwindow* window, const std::string& preferredDevice = "");
    void SetVertices(const std::vector<Vertex>& vertices);
    void Render();
    void Zoom(float delta);
    void Pan(float dx, float dy);
    void OnMouseButton(int button, int action, int mods);
    void OnCursorPos(double x, double y);

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
    wgpu::BindGroupLayout bindGroupLayout;

    // Camera State
    float zoomLevel = 1.0f;
    float translationX = 0.0f;
    float translationY = 0.0f;
    float rotationX = 0.0f;
    float rotationY = 0.0f;
    
    // Mouse State
    bool isDraggingLeft = false;
    bool isDraggingRight = false;
    double lastMouseX = 0.0;
    double lastMouseY = 0.0;
    GLFWwindow* window = nullptr;

    bool InitDevice(const std::string& preferredDevice);
    bool InitSurface(GLFWwindow* window);
    bool InitPipeline();
};
