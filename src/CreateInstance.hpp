#pragma once

#include <webgpu/webgpu.h>
#include "Engine.hpp"

void CreateInstance(ES::Engine::Core &core) {
	ES::Utils::Log::Info("Creating WebGPU instance...");

	wgpu::InstanceDescriptor desc(wgpu::Default);

	auto &instance = core.RegisterResource(wgpu::Instance(wgpuCreateInstance(&desc)));

	if (instance == nullptr) throw std::runtime_error("Could not create WebGPU instance");

	ES::Utils::Log::Info(fmt::format("WebGPU instance created: {}", static_cast<void*>(instance)));
}
