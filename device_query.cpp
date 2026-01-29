#include <webgpu/webgpu_cpp.h>
#include <dawn/native/DawnNative.h>
#include <iostream>
#include <vector>
#include <string>
#include <cstring>

/**
 * Modern Dawn Example using wgpu::AdapterInfo (v2026.01 compatible)
 */
int main()
{
    dawn::native::Instance nativeInstance;

    // Dawn固有の拡張を使用して、システム上の物理GPUを取得
    auto adapters = nativeInstance.EnumerateAdapters();

    if (adapters.empty())
    {
        std::cerr << "No adapters found. Check your build configuration (DAWN_ENABLE_BACKEND_VULKAN etc.)" << std::endl;
        return 1;
    }

    for (const auto &adapter : adapters)
    {
        // 1. Get the underlying C handle (WGPUAdapter)
        WGPUAdapter cAdapter = adapter.Get();

        // 2. Prepare AdapterInfo
        // In the latest WebGPU, properties are unified into AdapterInfo
        wgpu::AdapterInfo info = {};

        // Use the C API to fill the C++ struct (they are layout-compatible)
        wgpuAdapterGetInfo(cAdapter, reinterpret_cast<WGPUAdapterInfo *>(&info));

        // 3. StringView to std::string conversion helper
        // Modern WebGPU uses StringView for all text fields
        auto decodeSV = [](wgpu::StringView sv) -> std::string
        {
            if (!sv.data)
                return "N/A";
            // If length is WGPU_STRLEN (0xffffffff...), it's null-terminated
            if (sv.length == WGPU_STRLEN)
                return std::string(sv.data);
            return std::string(sv.data, sv.length);
        };

        std::cout << "--- Adapter Found ---" << std::endl;
        std::cout << "Device:       " << decodeSV(info.device) << std::endl;
        std::cout << "Vendor:       " << decodeSV(info.vendor) << std::endl;
        std::cout << "Architecture: " << decodeSV(info.architecture) << std::endl;
        std::cout << "Description:  " << decodeSV(info.description) << std::endl;

        std::cout << "Backend Type: ";
        switch (info.backendType)
        {
        case wgpu::BackendType::D3D12:
            std::cout << "D3D12";
            break;
        case wgpu::BackendType::Metal:
            std::cout << "Metal";
            break;
        case wgpu::BackendType::Vulkan:
            std::cout << "Vulkan";
            break;
        case wgpu::BackendType::OpenGL:
            std::cout << "OpenGL";
            break;
        case wgpu::BackendType::OpenGLES:
            std::cout << "OpenGLES";
            break;
        default:
            std::cout << "Unknown";
            break;
        }
        std::cout << std::endl;

        // 4. Memory/Limits Check (Important for GPGPU)
        WGPULimits limits = {};
        if (wgpuAdapterGetLimits(cAdapter, &limits))
        {
            std::cout << "maxBufferSize: "
                      << limits.maxBufferSize / 1024 / 1024 << " M bytes" << std::endl;
            std::cout << "maxStorageBufferBindingSize: "
                      << limits.maxStorageBufferBindingSize / 1024 / 1024 << " M bytes" << std::endl;
            std::cout << "maxBindGroups: "
                      << limits.maxBindGroups << std::endl;
            std::cout << "maxComputeWorkgroupStorageSize: "
                      << limits.maxComputeWorkgroupStorageSize / 1024 << " K bytes" << std::endl;
            std::cout << "maxComputeWorkgroupSizeX: "
                      << limits.maxComputeWorkgroupSizeX << std::endl;
            std::cout << "maxComputeWorkgroupSizeY: "
                      << limits.maxComputeWorkgroupSizeY << std::endl;
            std::cout << "maxComputeWorkgroupSizeZ: "
                      << limits.maxComputeWorkgroupSizeZ << std::endl;
        }

        std::cout << "----------------------" << std::endl;
    }

    return 0;
}
