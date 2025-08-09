#include "SetupQueueOnSubmittedWorkDone.hpp"
#include "webgpu.hpp"

namespace ES::Plugin::WebGPU::System {

void SetupQueueOnSubmittedWorkDone(ES::Engine::Core &core)
{
	auto &queue = core.GetResource<wgpu::Queue>();

	auto onQueueWorkDone = [](WGPUQueueWorkDoneStatus status, WGPU_NULLABLE void* userdata1, WGPU_NULLABLE void* userdata2) {
		ES::Utils::Log::Debug(fmt::format("Queued work finished with status: {:x}", static_cast<uint32_t>(status)));
	};
	wgpu::QueueWorkDoneCallbackInfo callbackInfo(wgpu::Default);
	callbackInfo.callback = onQueueWorkDone;
	queue.onSubmittedWorkDone(callbackInfo);
}
}
