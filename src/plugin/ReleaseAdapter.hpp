#pragma once

#include <webgpu/webgpu.h>
#include "Engine.hpp"

void ReleaseAdapter(ES::Engine::Core &core)
{
	auto &adapter = core.GetResource<wgpu::Adapter>();

	if (adapter == nullptr) throw std::runtime_error("WebGPU adapter is already released or was never created.");

	ES::Utils::Log::Debug(fmt::format("Releasing WebGPU adapter: {}", static_cast<void*>(adapter)));
	adapter.release();
	adapter = nullptr;
	// TODO: Remove the adapter from the core resources (#252)
	ES::Utils::Log::Debug("WebGPU adapter released.");
}
