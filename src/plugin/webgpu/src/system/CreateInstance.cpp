#include "CreateInstance.hpp"
#include "WebGPU.hpp"

namespace ES::Plugin::WebGPU::System {

void CreateInstance(ES::Engine::Core &core) {
	ES::Utils::Log::Debug("Creating WebGPU instance...");

	wgpu::InstanceDescriptor desc(wgpu::Default);

	auto &instance = core.RegisterResource(wgpu::Instance(wgpuCreateInstance(&desc)));

	if (instance == nullptr) throw std::runtime_error("Could not create WebGPU instance");

	ES::Utils::Log::Debug(fmt::format("WebGPU instance created: {}", static_cast<void*>(instance)));
}
}
