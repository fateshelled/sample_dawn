#include "Renderer.h"
#include <webgpu/webgpu_glfw.h>
#include <dawn/native/DawnNative.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <algorithm>
#include <cfloat>

Renderer::Renderer() {}

Renderer::~Renderer() {}

bool Renderer::Initialize(GLFWwindow* window) {
    if (!InitDevice()) return false;
    if (!InitSurface(window)) return false;
    if (!InitPipeline()) return false;
    return true;
}

bool Renderer::InitDevice() {
    instance = wgpu::CreateInstance();
    if (!instance) return false;

    dawn::native::Instance nativeInstance;
    auto adapters = nativeInstance.EnumerateAdapters();
    if (adapters.empty()) return false;
    
    WGPUDevice cDevice = adapters[0].CreateDevice();
    device = wgpu::Device::Acquire(cDevice);
    queue = device.GetQueue();
    
    return true;
}

bool Renderer::InitSurface(GLFWwindow* window) {
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

@vertex
fn vs_main(input: VertexInput) -> VertexOutput {
    var output: VertexOutput;
    output.position = vec4<f32>(input.position.xy, input.position.z * 0.5 + 0.5, 1.0);
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
    pass.SetVertexBuffer(0, vertexBuffer);
    pass.Draw(vertexCount);
    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);
    surface.Present();
}
