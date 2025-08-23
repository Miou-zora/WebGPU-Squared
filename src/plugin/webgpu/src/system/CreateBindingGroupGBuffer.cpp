#include "CreateBindingGroupGBuffer.hpp"
#include "structs.hpp"

namespace ES::Plugin::WebGPU::System {
void CreateBindingGroupGBuffer(ES::Engine::Core &core) {
    auto &device = core.GetResource<wgpu::Device>();
    auto &pipelineData = core.GetResource<Pipelines>().renderPipelines["GBuffer"];
    auto &bindGroups = core.GetResource<BindGroups>();
    if (device == nullptr) throw std::runtime_error("WebGPU device is not created, cannot create binding group.");

    wgpu::BindGroupEntry bindingUniforms(wgpu::Default); bindingUniforms.binding = 0; bindingUniforms.buffer = uniformsBuffer; bindingUniforms.size = sizeof(Uniforms);
    wgpu::BindGroupEntry bindingCamera(wgpu::Default); bindingCamera.binding = 1; bindingCamera.buffer = cameraBuffer; bindingCamera.size = sizeof(Camera);

    std::array<wgpu::BindGroupEntry, 2> bindings = { bindingUniforms, bindingCamera };
    wgpu::BindGroupDescriptor bindGroupDesc(wgpu::Default);
    bindGroupDesc.layout = pipelineData.bindGroupLayouts[0];
    bindGroupDesc.entryCount = bindings.size();
    bindGroupDesc.entries = bindings.data();
    bindGroupDesc.label = wgpu::StringView("GBuffer Binding Group");
    auto bg1 = device.createBindGroup(bindGroupDesc);
    if (bg1 == nullptr) throw std::runtime_error("Could not create WebGPU bind group");
    bindGroups.groups["GBuffer"] = bg1;
}
}
