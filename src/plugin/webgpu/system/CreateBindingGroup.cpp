#include "CreateBindingGroup.hpp"
#include "WebGPU.hpp"

namespace ES::Plugin::WebGPU::System {
void CreateBindingGroup(ES::Engine::Core &core)
{
	auto &device = core.GetResource<wgpu::Device>();
	auto &pipelineData = core.GetResource<Pipelines>().renderPipelines["3D"];
	//TODO: Put this in a separate system
	//TODO: Should we separate this from pipelineData?
	auto &bindGroups = core.RegisterResource(BindGroups());
	auto &lights = core.GetResource<std::vector<Light>>();

	if (device == nullptr) throw std::runtime_error("WebGPU device is not created, cannot create binding group.");

	// Create a binding
	wgpu::BindGroupEntry binding(wgpu::Default);
	binding.binding = 0;
	binding.buffer = uniformBuffer;
	binding.size = sizeof(MyUniforms);

	std::array<wgpu::BindGroupEntry, 1> bindings = { binding };

	wgpu::BindGroupDescriptor bindGroupDesc(wgpu::Default);
	bindGroupDesc.layout = pipelineData.bindGroupLayouts[0];
	bindGroupDesc.entryCount = bindings.size();
	bindGroupDesc.entries = bindings.data();
	bindGroupDesc.label = wgpu::StringView("My Bind Group");
	auto bg1 = device.createBindGroup(bindGroupDesc);

	if (bg1 == nullptr) throw std::runtime_error("Could not create WebGPU bind group");

	bindGroups.groups["1"] = bg1;

	wgpu::BindGroupEntry bindingLights(wgpu::Default);
	bindingLights.binding = 0;
	bindingLights.buffer = lightsBuffer;
	bindingLights.size = sizeof(Light) + sizeof(uint32_t) + 12 /* (padding) */; // TODO: Resize when adding a new light

	std::array<wgpu::BindGroupEntry, 1> lightsBindings = { bindingLights };

	wgpu::BindGroupDescriptor bindGroupLightsDesc(wgpu::Default);
	bindGroupLightsDesc.layout = pipelineData.bindGroupLayouts[1];
	bindGroupLightsDesc.entryCount = lightsBindings.size();
	bindGroupLightsDesc.entries = lightsBindings.data();
	bindGroupLightsDesc.label = wgpu::StringView("Lights Bind Group");
	auto bg2 = device.createBindGroup(bindGroupLightsDesc);

	if (bg2 == nullptr) throw std::runtime_error("Could not create WebGPU bind group");

	bindGroups.groups["2"] = bg2;
}
}
