#pragma once

#include <webgpu/webgpu.h>
#include "Engine.hpp"

void SetupQueueOnSubmittedWorkDone(ES::Engine::Core &core)
{
	auto &queue = core.GetResource<wgpu::Queue>();

	auto onQueueWorkDone = [](WGPUQueueWorkDoneStatus status, WGPU_NULLABLE void* userdata1, WGPU_NULLABLE void* userdata2) {
		ES::Utils::Log::Info(fmt::format("Queued work finished with status: {:x}", static_cast<uint32_t>(status)));
	};
	wgpu::QueueWorkDoneCallbackInfo callbackInfo(wgpu::Default);
	callbackInfo.callback = onQueueWorkDone;
	wgpuQueueOnSubmittedWorkDone(queue, callbackInfo);
}
