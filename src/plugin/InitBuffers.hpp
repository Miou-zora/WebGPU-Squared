#pragma once

#include "structs.hpp"
#include "Engine.hpp"


void InitializeBuffers(ES::Engine::Core &core)
{
	wgpu::Queue &queue = core.GetResource<wgpu::Queue>();
	wgpu::Device &device = core.GetResource<wgpu::Device>();
	auto &lights = core.GetResource<std::vector<Light>>();


	if (queue == nullptr) throw std::runtime_error("WebGPU queue is not created, cannot initialize buffers.");
	if (device == nullptr) throw std::runtime_error("WebGPU device is not created, cannot initialize buffers.");

	wgpu::BufferDescriptor bufferDesc(wgpu::Default);
	bufferDesc.size = sizeof(MyUniforms);
	bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
	uniformBuffer = device.createBuffer(bufferDesc);

	wgpu::BufferDescriptor lightsBufferDesc(wgpu::Default);
	lightsBufferDesc.size = sizeof(Light) + sizeof(uint32_t) + 12 /* (padding) */;
	lightsBufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage;
	lightsBufferDesc.label = wgpu::StringView("Lights Buffer");
	lightsBuffer = device.createBuffer(lightsBufferDesc);

	// Upload the initial value of the uniforms
	MyUniforms uniforms;
	uniforms.time = 1.0f;
	uniforms.color = { 1.0f, 1.0f, 1.0f, 1.0f };
	float near_value = 0.001f;
	float far_value = 100.0f;
	float ratio = 800.0f / 800.0f;
	float fov = glm::radians(45.0f);
	uniforms.projectionMatrix = glm::perspective(fov, ratio, near_value, far_value);
	queue.writeBuffer(uniformBuffer, 0, &uniforms, sizeof(uniforms));

	uint32_t lightsCount = static_cast<uint32_t>(lights.size());
	queue.writeBuffer(lightsBuffer, 0, &lightsCount, sizeof(uint32_t));
	queue.writeBuffer(lightsBuffer, sizeof(uint32_t), lights.data(), sizeof(Light) * lights.size());
}
