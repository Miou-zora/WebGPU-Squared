#include "CreateBindingGroupGBuffer.hpp"
#include "structs.hpp"

namespace ES::Plugin::WebGPU::System {
void CreateBindingGroupGBuffer(ES::Engine::Core &core) {
    auto &device = core.GetResource<wgpu::Device>();
    auto &pipelineData = core.GetResource<Pipelines>().renderPipelines["GBuffer"];
    auto &bindGroups = core.GetResource<BindGroups>();
    if (device == nullptr) throw std::runtime_error("WebGPU device is not created, cannot create binding group.");

    wgpu::BindGroupEntry bindingCamera(wgpu::Default);
    bindingCamera.binding = 0;
    bindingCamera.buffer = cameraBuffer;
    bindingCamera.size = sizeof(Camera);

    std::array<wgpu::BindGroupEntry, 1> bindingsCamera = { bindingCamera };
    wgpu::BindGroupDescriptor bindGroupDesc(wgpu::Default);
    bindGroupDesc.layout = pipelineData.bindGroupLayouts[0];
    bindGroupDesc.entryCount = bindingsCamera.size();
    bindGroupDesc.entries = bindingsCamera.data();
    bindGroupDesc.label = wgpu::StringView("GBuffer Binding Group Camera");
    auto bg1 = device.createBindGroup(bindGroupDesc);
    if (bg1 == nullptr) throw std::runtime_error("Could not create WebGPU bind group");
    bindGroups.groups["GBuffer"] = bg1;

    wgpu::BindGroupEntry bindingUniforms(wgpu::Default);
    bindingUniforms.binding = 0;
    bindingUniforms.buffer = uniformsBuffer;
    bindingUniforms.size = sizeof(Uniforms);

    std::array<wgpu::BindGroupEntry, 1> bindingsUniforms = { bindingUniforms };
    bindGroupDesc.layout = pipelineData.bindGroupLayouts[2];
    bindGroupDesc.entryCount = bindingsUniforms.size();
    bindGroupDesc.entries = bindingsUniforms.data();
    bindGroupDesc.label = wgpu::StringView("GBuffer Binding Group Uniforms");
    auto bg2 = device.createBindGroup(bindGroupDesc);
    if (bg2 == nullptr) throw std::runtime_error("Could not create WebGPU bind group");
    bindGroups.groups["GBufferUniforms"] = bg2;
}
}
