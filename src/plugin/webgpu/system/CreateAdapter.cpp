#include "CreateAdapter.hpp"
#include "WebGPU.hpp"

namespace ES::Plugin::WebGPU::System {

void CreateAdapter(ES::Engine::Core &core) {
	ES::Utils::Log::Debug("Requesting adapter...");

	auto &instance = core.GetResource<wgpu::Instance>();
	auto &surface = core.GetResource<wgpu::Surface>();

	if (instance == nullptr) throw std::runtime_error("WebGPU instance is not created, cannot request adapter");
	if (surface == nullptr) throw std::runtime_error("Surface is not created, cannot request adapter.");

	wgpu::RequestAdapterOptions adapterOpts(wgpu::Default);
	adapterOpts.compatibleSurface = surface;

	wgpu::Adapter adapter = core.RegisterResource(instance.requestAdapter(adapterOpts));

	if (adapter == nullptr) throw std::runtime_error("Could not get WebGPU adapter");
	ES::Utils::Log::Debug(fmt::format("Got adapter: {}", static_cast<void*>(adapter)));
}

}
