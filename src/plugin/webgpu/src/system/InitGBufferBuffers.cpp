#include "InitGBufferBuffers.hpp"
#include "structs.hpp"

namespace ES::Plugin::WebGPU::System {
void InitGBufferBuffers(ES::Engine::Core &core) {
    wgpu::Queue &queue = core.GetResource<wgpu::Queue>();
    wgpu::Device &device = core.GetResource<wgpu::Device>();
    if (queue == nullptr) throw std::runtime_error("WebGPU queue is not created, cannot initialize buffers.");
    if (device == nullptr) throw std::runtime_error("WebGPU device is not created, cannot initialize buffers.");

    wgpu::BufferDescriptor bufferDesc(wgpu::Default);
    bufferDesc.size = sizeof(Camera);
    bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
    bufferDesc.label = wgpu::StringView("Camera Buffer for GBuffer");
    cameraBuffer = device.createBuffer(bufferDesc);

    Camera camera;
    camera.viewProjectionMatrix = glm::mat4(1.0f);
    camera.invViewProjectionMatrix = glm::mat4(1.0f);
    queue.writeBuffer(cameraBuffer, 0, &camera, sizeof(camera));

    bufferDesc.size = sizeof(Uniforms);
    bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage;
    bufferDesc.label = wgpu::StringView("Uniforms Buffer for GBuffer");
    uniformsBuffer = device.createBuffer(bufferDesc);

    Uniforms uniforms;
    uniforms.modelMatrix = glm::mat4(1.0f);
    uniforms.normalModelMatrix = glm::mat4(1.0f);
    queue.writeBuffer(uniformsBuffer, 0, &uniforms, sizeof(uniforms));
}
}
