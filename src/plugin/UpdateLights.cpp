#include "webgpu.hpp"
#include "UpdateLights.hpp"
#include "structs.hpp"

namespace ES::Plugin::WebGPU::Utils {

void UpdateLights(ES::Engine::Core &core)
{
    auto &device = core.GetResource<wgpu::Device>();
    auto &pipelineData = core.GetResource<Pipelines>().renderPipelines["3D"];
    auto &bindGroups = core.GetResource<BindGroups>();
    auto &queue = core.GetResource<wgpu::Queue>();
    auto &lights = core.GetResource<std::vector<Light>>();

    lightsBuffer.destroy();
    lightsBuffer.release();

    wgpu::BufferDescriptor lightsBufferDesc(wgpu::Default);
    lightsBufferDesc.size = sizeof(Light) * std::max(lights.size(), size_t(1)) + sizeof(uint32_t) + 12 /* (padding) */;
    lightsBufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage;
    lightsBufferDesc.label = wgpu::StringView("Lights Buffer");
    lightsBuffer = device.createBuffer(lightsBufferDesc);

    uint32_t lightsCount = static_cast<uint32_t>(lights.size());
    queue.writeBuffer(lightsBuffer, 0, &lightsCount, sizeof(uint32_t));
    queue.writeBuffer(lightsBuffer, sizeof(uint32_t), lights.data(), sizeof(Light) * lights.size());

    bindGroups.groups["2"].release();

    wgpu::BindGroupEntry bindingLights(wgpu::Default);
    bindingLights.binding = 0;
    bindingLights.buffer = lightsBuffer;
    bindingLights.size = sizeof(Light) * std::max(lights.size(), size_t(1)) + sizeof(uint32_t) + 12 /* (padding) */; // TODO: Resize when adding a new light

    std::array<wgpu::BindGroupEntry, 1> lightsBindings = { bindingLights };

    wgpu::BindGroupDescriptor bindGroupLightsDesc(wgpu::Default);
    bindGroupLightsDesc.layout = pipelineData.bindGroupLayouts[1];
    bindGroupLightsDesc.entryCount = lightsBindings.size();
    bindGroupLightsDesc.entries = lightsBindings.data();
    bindGroupLightsDesc.label = wgpu::StringView("Lights Bind Group");
    auto bg = device.createBindGroup(bindGroupLightsDesc);

    bindGroups.groups["2"] = bg;
}
}
