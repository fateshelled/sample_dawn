#include "Renderer.h"
#include <webgpu/webgpu_glfw.h>
#include <dawn/native/DawnNative.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <algorithm>
#include <cfloat>

Renderer::Renderer() {}

Renderer::~Renderer() {}

bool Renderer::Initialize(GLFWwindow* window, const std::string& preferredDevice) {
    if (!InitDevice(preferredDevice)) return false;
    
    // Create uniform buffer
    wgpu::BufferDescriptor bufferDesc = {};
    bufferDesc.size = sizeof(Uniforms);
    bufferDesc.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
    uniformBuffer = device.CreateBuffer(&bufferDesc);
    UpdateUniforms();

    if (!InitSurface(window)) return false;
    if (!InitPipeline()) return false;
    return true;
}

bool Renderer::InitDevice(const std::string& preferredDevice) {
    instance = wgpu::CreateInstance();
    if (!instance) return false;

    dawn::native::Instance nativeInstance;
    auto adapters = nativeInstance.EnumerateAdapters();
    if (adapters.empty()) return false;

    WGPUDevice cDevice = nullptr;
    std::string selectedName;

    // Helper to decode StringView
    auto decodeSV = [](wgpu::StringView sv) -> std::string {
        if (!sv.data) return "";
        if (sv.length == WGPU_STRLEN) return std::string(sv.data);
        return std::string(sv.data, sv.length);
    };

    if (!preferredDevice.empty()) {
        for (const auto& adapter : adapters) {
            wgpu::AdapterInfo info = {};
            // dawn::native::Adapter::Get() returns WGPUAdapter (C handle)
            WGPUAdapter cAdapter = adapter.Get();
            wgpuAdapterGetInfo(cAdapter, reinterpret_cast<WGPUAdapterInfo*>(&info));
            
            std::string deviceName = decodeSV(info.device);
            if (deviceName.find(preferredDevice) != std::string::npos) {
                // dawn::native::Adapter is const, but CreateDevice is non-const. 
                // We need to const_cast or copy since EnumerateAdapters returns vector of objects.
                // Actually, let's just make a mutable copy or cast since we need to call non-const method.
                // dawn::native::Adapter is a light wrapper.
                auto mutableAdapter = adapter;
                cDevice = mutableAdapter.CreateDevice();
                selectedName = deviceName;
                std::cout << "Selected preferred device: " << selectedName << std::endl;
                break;
            }
        }
        if (!cDevice) {
            std::cerr << "Preferred device '" << preferredDevice << "' not found. Falling back to default." << std::endl;
        }
    }

    if (!cDevice) {
        auto mutableAdapter = adapters[0];
        cDevice = mutableAdapter.CreateDevice();
        // Get name for logging
        wgpu::AdapterInfo info = {};
        WGPUAdapter cAdapter = adapters[0].Get();
        wgpuAdapterGetInfo(cAdapter, reinterpret_cast<WGPUAdapterInfo*>(&info));
        selectedName = decodeSV(info.device);
        std::cout << "Selected default device: " << selectedName << std::endl;
    }

    device = wgpu::Device::Acquire(cDevice);
    queue = device.GetQueue();
    
    return true;
}

bool Renderer::InitSurface(GLFWwindow* window) {
    this->window = window;
    surface = wgpu::glfw::CreateSurfaceForWindow(instance, window);
    if (!surface) return false;

    wgpu::SurfaceConfiguration config = {};
    config.device = device;
    config.format = format;
    config.width = 800;
    config.height = 600;
    config.usage = wgpu::TextureUsage::RenderAttachment;
    surface.Configure(&config);

    return true;
}

static const char* shaderCode = R"(
struct VertexInput {
    @location(0) position: vec3<f32>,
    @location(1) intensity: f32,
};

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) color: vec4<f32>,
};

struct Uniforms {
    scale : f32,
    translation : vec2<f32>,
};
@group(0) @binding(0) var<uniform> uniforms : Uniforms;

@vertex
fn vs_main(input: VertexInput) -> VertexOutput {
    var output: VertexOutput;
    // Apply zoom scale. Note: input is already normalized to [-0.9, 0.9] range roughly.
    // We scale x and y by zoom level.
    output.position = vec4<f32>(input.position.xy * uniforms.scale + uniforms.translation, input.position.z * 0.5 + 0.5, 1.0);
    let c = input.intensity / 255.0;
    output.color = vec4<f32>(c, c, c, 1.0);
    return output;
}

@fragment
fn fs_main(input: VertexOutput) -> @location(0) vec4<f32> {
    return input.color;
}
)";

bool Renderer::InitPipeline() {
    wgpu::ShaderModuleDescriptor shaderDesc = {};
    wgpu::ShaderSourceWGSL wgslDesc = {};
    wgslDesc.code = shaderCode;
    shaderDesc.nextInChain = &wgslDesc;
    wgpu::ShaderModule shaderModule = device.CreateShaderModule(&shaderDesc);

    wgpu::RenderPipelineDescriptor pipelineDesc = {};
    
    wgpu::VertexAttribute attributes[2];
    attributes[0].format = wgpu::VertexFormat::Float32x3;
    attributes[0].offset = 0;
    attributes[0].shaderLocation = 0;
    attributes[1].format = wgpu::VertexFormat::Float32;
    attributes[1].offset = sizeof(float) * 3;
    attributes[1].shaderLocation = 1;

    wgpu::VertexBufferLayout vertexBufferLayout = {};
    vertexBufferLayout.arrayStride = sizeof(Vertex);
    vertexBufferLayout.stepMode = wgpu::VertexStepMode::Vertex;
    vertexBufferLayout.attributeCount = 2;
    vertexBufferLayout.attributes = attributes;

    pipelineDesc.vertex.module = shaderModule;
    pipelineDesc.vertex.entryPoint = "vs_main";
    pipelineDesc.vertex.bufferCount = 1;
    pipelineDesc.vertex.buffers = &vertexBufferLayout;

    wgpu::ColorTargetState colorTarget = {};
    colorTarget.format = format;
    wgpu::FragmentState fragmentState = {};
    fragmentState.module = shaderModule;
    fragmentState.entryPoint = "fs_main";
    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTarget;
    pipelineDesc.fragment = &fragmentState;

    pipelineDesc.primitive.topology = wgpu::PrimitiveTopology::PointList;

    // Create BindGroupLayout and BindGroup
    wgpu::BindGroupLayoutEntry bindingLayout = {};
    bindingLayout.binding = 0;
    bindingLayout.visibility = wgpu::ShaderStage::Vertex;
    bindingLayout.buffer.type = wgpu::BufferBindingType::Uniform;
    bindingLayout.buffer.minBindingSize = sizeof(Uniforms);

    wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc = {};
    bindGroupLayoutDesc.entryCount = 1;
    bindGroupLayoutDesc.entries = &bindingLayout;
    bindGroupLayout = device.CreateBindGroupLayout(&bindGroupLayoutDesc);

    wgpu::PipelineLayoutDescriptor pipelineLayoutDesc = {};
    pipelineLayoutDesc.bindGroupLayoutCount = 1;
    pipelineLayoutDesc.bindGroupLayouts = &bindGroupLayout;
    wgpu::PipelineLayout pipelineLayout = device.CreatePipelineLayout(&pipelineLayoutDesc);
    pipelineDesc.layout = pipelineLayout;

    wgpu::BindGroupEntry binding = {};
    binding.binding = 0;
    binding.buffer = uniformBuffer;
    binding.offset = 0;
    binding.size = sizeof(Uniforms);

    wgpu::BindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = bindGroupLayout;
    bindGroupDesc.entryCount = 1;
    bindGroupDesc.entries = &binding;
    bindGroup = device.CreateBindGroup(&bindGroupDesc);

    pipeline = device.CreateRenderPipeline(&pipelineDesc);
    return true;
}

void Renderer::SetVertices(const std::vector<Vertex>& vertices) {
    vertexCount = static_cast<uint32_t>(vertices.size());

    // Compute bounding box
    float minX = FLT_MAX, maxX = -FLT_MAX;
    float minY = FLT_MAX, maxY = -FLT_MAX;
    float minZ = FLT_MAX, maxZ = -FLT_MAX;
    for (const auto& v : vertices) {
        minX = std::min(minX, v.x); maxX = std::max(maxX, v.x);
        minY = std::min(minY, v.y); maxY = std::max(maxY, v.y);
        minZ = std::min(minZ, v.z); maxZ = std::max(maxZ, v.z);
    }

    // Center and scale to fit within [-0.9, 0.9]
    float centerX = (minX + maxX) / 2.0f;
    float centerY = (minY + maxY) / 2.0f;
    float centerZ = (minZ + maxZ) / 2.0f;
    float maxExtent = std::max({maxX - minX, maxY - minY, maxZ - minZ});
    float scale = 1.8f / maxExtent;

    std::vector<Vertex> normalized(vertices.size());
    for (size_t i = 0; i < vertices.size(); ++i) {
        normalized[i].x = (vertices[i].x - centerX) * scale;
        normalized[i].y = (vertices[i].y - centerY) * scale;
        normalized[i].z = (vertices[i].z - centerZ) * scale;
        normalized[i].intensity = vertices[i].intensity;
    }

    wgpu::BufferDescriptor bufferDesc = {};
    bufferDesc.size = normalized.size() * sizeof(Vertex);
    bufferDesc.usage = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst;
    vertexBuffer = device.CreateBuffer(&bufferDesc);
    queue.WriteBuffer(vertexBuffer, 0, normalized.data(), bufferDesc.size);
}

void Renderer::Render() {
    wgpu::SurfaceTexture surfaceTexture;
    surface.GetCurrentTexture(&surfaceTexture);
    if (!surfaceTexture.texture) return;
    
    wgpu::RenderPassColorAttachment colorAttachment = {};
    colorAttachment.view = surfaceTexture.texture.CreateView();
    colorAttachment.loadOp = wgpu::LoadOp::Clear;
    colorAttachment.storeOp = wgpu::StoreOp::Store;
    colorAttachment.clearValue = {0.1f, 0.1f, 0.2f, 1.0f};

    wgpu::RenderPassDescriptor renderPassDesc = {};
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.colorAttachments = &colorAttachment;

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);
    pass.SetPipeline(pipeline);
    pass.SetBindGroup(0, bindGroup);
    pass.SetVertexBuffer(0, vertexBuffer);
    pass.Draw(vertexCount);
    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);
    surface.Present();
}

void Renderer::Zoom(float delta) {
    zoomLevel += delta * 0.1f;
    if (zoomLevel < 0.1f) zoomLevel = 0.1f;
    if (zoomLevel > 10.0f) zoomLevel = 10.0f;
    UpdateUniforms();
}

void Renderer::UpdateUniforms() {
    Uniforms u;
    u.scale = zoomLevel;
    u.padding1 = 0.0f;
    u.translation[0] = translationX;
    u.translation[1] = translationY;
    queue.WriteBuffer(uniformBuffer, 0, &u, sizeof(Uniforms));
}

void Renderer::Pan(float dx, float dy) {
    translationX += dx;
    translationY += dy;
    UpdateUniforms();
}

void Renderer::OnMouseButton(int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            isDragging = true;
            if (window) {
                glfwGetCursorPos(window, &lastMouseX, &lastMouseY);
            }
        } else if (action == GLFW_RELEASE) {
            isDragging = false;
        }
    }
}

void Renderer::OnCursorPos(double x, double y) {
    if (isDragging) {
        float deltaX = static_cast<float>(x - lastMouseX);
        float deltaY = static_cast<float>(y - lastMouseY);
        lastMouseX = x;
        lastMouseY = y;

        int width = 800, height = 600;
        if (window) {
            glfwGetWindowSize(window, &width, &height);
        }

        // Convert pixel delta to clip space delta
        // Screen is [-1, 1] in width and height.
        // Scale factor: 2.0 / dimension
        // Invert Y because screen Y is top-down (0 at top), Clip Y is bottom-up (-1 at bottom, 1 at top)
        float ndcDeltaX = deltaX * (2.0f / width);
        float ndcDeltaY = deltaY * (-2.0f / height); 

        Pan(ndcDeltaX, ndcDeltaY);
    }
}
