#include "ReleasePipeline.hpp"
#include "structs.hpp"

namespace ES::Plugin::WebGPU::System {
void ReleasePipeline(ES::Engine::Core &core)
{
	ES::Utils::Log::Debug("Releasing pipelines...");
	Pipelines &pipelines = core.GetResource<Pipelines>();

	for (auto &pair : pipelines.renderPipelines) {
		if (pair.second.pipeline != nullptr) {
			pair.second.pipeline.release();
			pair.second.pipeline = nullptr;
		}
	}
	ES::Utils::Log::Debug("Pipelines released.");
}
}
