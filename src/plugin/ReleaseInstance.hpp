#pragma once

#include <webgpu/webgpu.h>
#include "Engine.hpp"

void ReleaseInstance(ES::Engine::Core &core) {
	wgpu::Instance &instance = core.GetResource<wgpu::Instance>();

	if (instance == nullptr) throw std::runtime_error("WebGPU instance is already released or was never created.");

	ES::Utils::Log::Debug(fmt::format("Releasing WebGPU instance: {}", static_cast<void*>(instance)));

	instance.release();
	instance = nullptr;
	// TODO: Remove the instance from the core resources (#252)
}
