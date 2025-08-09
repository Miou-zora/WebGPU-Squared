#include "webgpu.hpp"
#include "ReleaseDevice.hpp"

namespace ES::Plugin::WebGPU::System {

void ReleaseDevice(ES::Engine::Core &core)
{
	wgpu::Device &device = core.GetResource<wgpu::Device>();

	if (device) {
		device.destroy();
		device.release();
		device = nullptr;
	}
}
}
