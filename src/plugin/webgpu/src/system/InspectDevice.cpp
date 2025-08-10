#include "InspectDevice.hpp"
#include "WebGPU.hpp"

namespace ES::Plugin::WebGPU::System {

void InspectDevice(ES::Engine::Core &core) {
    const wgpu::Device &device = core.GetResource<wgpu::Device>();
    if (device == nullptr) throw std::runtime_error("Device is not created, cannot inspect it.");

    wgpu::SupportedFeatures features(wgpu::Default);
    device.getFeatures(&features);

    ES::Utils::Log::Info("Device features:");
    for (size_t i = 0; i < features.featureCount; ++i) {
        ES::Utils::Log::Info(fmt::format(" - 0x{:x}", static_cast<uint32_t>(features.features[i])));
    }
    features.freeMembers();

    wgpu::Limits limits(wgpu::Default);
    limits.maxUniformBufferBindingSize = 16 * 4 * sizeof(float);
	limits.maxBindGroups = 3;
    limits.maxSampledTexturesPerShaderStage = 1;
    limits.maxInterStageShaderVariables = 8;

    bool success = device.getLimits(&limits) == wgpu::Status::Success;

	if (!success) throw std::runtime_error("Failed to get device limits");

	ES::Utils::Log::Info("Device limits:");
	ES::Utils::Log::Info(fmt::format(" - maxTextureDimension1D: {}", limits.maxTextureDimension1D));
	ES::Utils::Log::Info(fmt::format(" - maxTextureDimension2D: {}", limits.maxTextureDimension2D));
	ES::Utils::Log::Info(fmt::format(" - maxTextureDimension3D: {}", limits.maxTextureDimension3D));
	ES::Utils::Log::Info(fmt::format(" - maxTextureArrayLayers: {}", limits.maxTextureArrayLayers));
}
}
