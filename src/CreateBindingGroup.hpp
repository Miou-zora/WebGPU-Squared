#pragma once

#include <webgpu/webgpu.h>
#include "Engine.hpp"

void CreateBindingGroup(ES::Engine::Core &core)
{
	auto &device = core.GetResource<wgpu::Device>();
	auto &pipelineData = core.GetResource<Pipelines>().renderPipelines["3D"];
	//TODO: Put this in a separate system
	//TODO: Should we separate this from pipelineData?
	auto &bindGroups = core.RegisterResource(BindGroups());

	if (device == nullptr) throw std::runtime_error("WebGPU device is not created, cannot create binding group.");

	// Create a binding
	wgpu::BindGroupEntry binding(wgpu::Default);
	binding.binding = 0;
	binding.buffer = uniformBuffer;
	binding.size = sizeof(MyUniforms);

	wgpu::BindGroupEntry binding2(wgpu::Default);
	binding2.binding = 1;
	binding2.buffer = lightsBuffer;
	binding2.size = sizeof(Light) * MAX_LIGHTS;

	std::array<wgpu::BindGroupEntry, 2> bindings = { binding, binding2 };

	// A bind group contains one or multiple bindings
	wgpu::BindGroupDescriptor bindGroupDesc(wgpu::Default);
	bindGroupDesc.layout = pipelineData.bindGroupLayout;
	bindGroupDesc.entryCount = 2;
	bindGroupDesc.entries = bindings.data();
	bindGroupDesc.label = wgpu::StringView("My Bind Group");
	auto bg1 = device.createBindGroup(bindGroupDesc);

	if (bg1 == nullptr) throw std::runtime_error("Could not create WebGPU bind group");

	bindGroups.groups["1"] = bg1;
}
