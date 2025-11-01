#include "InitSamplerContainer.hpp"
#include "resource/SamplerContainer.hpp"

namespace ES::Plugin::WebGPU::System {

void InitSamplerContainer(ES::Engine::Core &core) {
	ES::Utils::Log::Debug("Creating SamplerContainer...");

	wgpu::Device device = core.GetResource<wgpu::Device>();

	if (device == nullptr)
		ES::Utils::Log::Error("WebGPU device is not created, cannot initialize SamplerContainer.");

	auto &samplerContainer = core.RegisterResource(Resource::SamplerContainer(device));	
	if (samplerContainer.GetDevice() == nullptr)
		ES::Utils::Log::Error("Could not create SamplerContainer with device");
	ES::Utils::Log::Debug("SamplerContainer created successfully");
}

}

