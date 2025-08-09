#include "CreateQueue.hpp"
#include "webgpu.hpp"

namespace ES::Plugin::WebGPU::System {

void CreateQueue(ES::Engine::Core &core) {
	ES::Utils::Log::Debug("Creating WebGPU queue...");

	const auto &device = core.GetResource<wgpu::Device>();

	if (device == nullptr) throw std::runtime_error("WebGPU device is not created, cannot create queue.");

	wgpu::Queue &queue = core.RegisterResource(wgpu::Queue(wgpuDeviceGetQueue(device)));

	if (queue == nullptr) throw std::runtime_error("Could not create WebGPU queue");

	ES::Utils::Log::Debug(fmt::format("WebGPU queue created: {}", static_cast<void*>(queue)));
}

}
