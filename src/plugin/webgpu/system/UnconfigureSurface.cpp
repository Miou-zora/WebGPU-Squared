#include "UnconfigureSurface.hpp"
#include "WebGPU.hpp"

namespace ES::Plugin::WebGPU::System {
void UnconfigureSurface(ES::Engine::Core &core) {
	auto &surface = core.GetResource<wgpu::Surface>();
	if (surface == nullptr) throw std::runtime_error("WebGPU surface is not created, cannot unconfigure it.");

	surface.unconfigure();
}
}
