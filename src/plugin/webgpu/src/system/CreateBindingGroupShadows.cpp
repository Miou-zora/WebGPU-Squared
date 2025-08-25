#include "CreateBindingGroupShadows.hpp"
#include "structs.hpp"

namespace ES::Plugin::WebGPU::System {
void CreateBindingGroupShadows(ES::Engine::Core &core)
{
	auto &device = core.GetResource<wgpu::Device>();
	auto &pipelineData = core.GetResource<Pipelines>().renderPipelines["2D"];
	auto &bindGroups = core.GetResource<BindGroups>();
	//TODO: Put this in a separate system
	//TODO: Should we separate this from pipelineData?

	if (device == nullptr) throw std::runtime_error("WebGPU device is not created, cannot create binding group.");

	wgpu::BindGroupEntry binding(wgpu::Default);
	binding.binding = 0;
	binding.buffer = uniform2DBuffer;
	binding.size = sizeof(Uniforms2D);

	std::array<wgpu::BindGroupEntry, 1> bindings = { binding };

	wgpu::BindGroupDescriptor bindGroupDesc(wgpu::Default);
	bindGroupDesc.layout = pipelineData.bindGroupLayouts[0];
	bindGroupDesc.entryCount = bindings.size();
	bindGroupDesc.entries = bindings.data();
	bindGroupDesc.label = wgpu::StringView("My Bind Group");
	auto bg1 = device.createBindGroup(bindGroupDesc);

	if (bg1 == nullptr) throw std::runtime_error("Could not create WebGPU bind group");

	bindGroups.groups["2D"] = bg1;
}
}
