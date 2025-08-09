#include "ReleaseSurface.hpp"
#include "webgpu.hpp"

namespace ES::Plugin::WebGPU::System {
void ReleaseSurface(ES::Engine::Core &core)
{
	wgpu::Surface &surface = core.GetResource<wgpu::Surface>();

	if (surface) {
		surface.unconfigure();
		surface.release();
		surface = nullptr;
	}
}
}
