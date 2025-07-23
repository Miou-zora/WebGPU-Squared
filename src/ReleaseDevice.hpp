#pragma once

#include "webgpu.hpp"
#include "Engine.hpp"

void ReleaseDevice(ES::Engine::Core &core)
{
	wgpu::Device &device = core.GetResource<wgpu::Device>();

	if (device) {
		wgpuDeviceRelease(device);
		device = nullptr;
	}
}
