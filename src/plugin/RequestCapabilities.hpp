#pragma once

#include <webgpu/webgpu.h>
#include "Engine.hpp"

void RequestCapabilities(ES::Engine::Core &core)
{
	const auto &adapter = core.GetResource<wgpu::Adapter>();
	const wgpu::Surface &surface = core.GetResource<wgpu::Surface>();

	if (adapter == nullptr) throw std::runtime_error("Adapter is not created, cannot request capabilities.");
	if (surface == nullptr) throw std::runtime_error("Surface is not created, cannot request capabilities.");

	wgpu::SurfaceCapabilities capabilities(wgpu::Default);

	surface.getCapabilities(adapter, &capabilities);

	core.RegisterResource(std::move(capabilities));
}
