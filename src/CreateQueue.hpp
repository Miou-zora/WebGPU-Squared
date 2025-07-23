#pragma once

#include <webgpu/webgpu.h>
#include "Engine.hpp"

void CreateQueue(ES::Engine::Core &core) {
	ES::Utils::Log::Info("Creating WebGPU queue...");

	const auto &device = core.GetResource<wgpu::Device>();

	if (device == nullptr) throw std::runtime_error("WebGPU device is not created, cannot create queue.");

	wgpu::Queue &queue = core.RegisterResource(wgpu::Queue(wgpuDeviceGetQueue(device)));

	if (queue == nullptr) throw std::runtime_error("Could not create WebGPU queue");

	ES::Utils::Log::Info(fmt::format("WebGPU queue created: {}", static_cast<void*>(queue)));
}
