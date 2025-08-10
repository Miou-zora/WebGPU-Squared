#include "ReleaseQueue.hpp"
#include "WebGPU.hpp"

namespace ES::Plugin::WebGPU::System {

void ReleaseQueue(ES::Engine::Core &core)
{
	wgpu::Queue &queue = core.GetResource<wgpu::Queue>();

	if (queue) {
		queue.release();
		queue = nullptr;
	}
}
}
