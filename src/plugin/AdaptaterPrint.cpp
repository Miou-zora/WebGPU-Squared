#include "AdaptaterPrint.hpp"

#include "webgpu.hpp"
#include "utils.hpp"
#include "Engine.hpp"

namespace ES::Plugin::WebGPU::System {

void AdaptaterPrintLimits(ES::Engine::Core &core)
{
	const auto &adapter = core.GetResource<wgpu::Adapter>();
	if (adapter == nullptr) throw std::runtime_error("Adapter is null, cannot print limits.");

	wgpu::Limits supportedLimits(wgpu::Default);

	wgpu::Status success = adapter.getLimits(&supportedLimits);

	if (success != wgpu::Status::Success) throw std::runtime_error("Failed to get adapter limits");

	ES::Utils::Log::Info("Adapter limits:");
	ES::Utils::Log::Info(fmt::format(" - maxTextureDimension1D: {}", supportedLimits.maxTextureDimension1D));
	ES::Utils::Log::Info(fmt::format(" - maxTextureDimension2D: {}", supportedLimits.maxTextureDimension2D));
	ES::Utils::Log::Info(fmt::format(" - maxTextureDimension3D: {}", supportedLimits.maxTextureDimension3D));
	ES::Utils::Log::Info(fmt::format(" - maxTextureArrayLayers: {}", supportedLimits.maxTextureArrayLayers));
}

void AdaptaterPrintFeatures(ES::Engine::Core &core)
{
	const auto &adapter = core.GetResource<wgpu::Adapter>();
	if (adapter == nullptr) throw std::runtime_error("Adapter is null, cannot print features.");

	wgpu::SupportedFeatures features(wgpu::Default);

	adapter.getFeatures(&features);

	ES::Utils::Log::Info("Adapter features:");
	for (size_t i = 0; i < features.featureCount; ++i) {
		ES::Utils::Log::Info(fmt::format(" - 0x{:x}", static_cast<uint32_t>(features.features[i])));
	}

	features.freeMembers();
}


void AdaptaterPrintProperties(ES::Engine::Core &core)
{
	const auto &adapter = core.GetResource<wgpu::Adapter>();
	if (adapter == nullptr) throw std::runtime_error("Adapter is null, cannot print properties.");

	wgpu::AdapterInfo properties(wgpu::Default);

	adapter.getInfo(&properties);

	ES::Utils::Log::Info("Adapter properties:");
	ES::Utils::Log::Info(fmt::format(" - vendorID: {}", properties.vendorID));
	ES::Utils::Log::Info(fmt::format(" - vendorName: {}", toStdStringView(properties.vendor)));
	ES::Utils::Log::Info(fmt::format(" - architecture: {}", toStdStringView(properties.architecture)));
	ES::Utils::Log::Info(fmt::format(" - deviceID: {}", properties.deviceID));
	ES::Utils::Log::Info(fmt::format(" - name: {}", toStdStringView(properties.device)));
	ES::Utils::Log::Info(fmt::format(" - driverDescription: {}", toStdStringView(properties.description)));
	ES::Utils::Log::Info(fmt::format(" - adapterType: 0x{:x}", static_cast<uint32_t>(properties.adapterType)));
	ES::Utils::Log::Info(fmt::format(" - backendType: 0x{:x}", static_cast<uint32_t>(properties.backendType)));

	properties.freeMembers();
}

}
