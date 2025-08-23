#include "GenerateSurfaceTexture.hpp"

#include "GetNextSurfaceViewData.hpp"
#include "structs.hpp"

namespace ES::Plugin::WebGPU::System {
void GenerateSurfaceTexture(ES::Engine::Core &core)
{
	wgpu::Surface &surface = core.GetResource<wgpu::Surface>();
	ES::Plugin::WebGPU::Util::GetNextSurfaceViewData(core, surface);
}
}
