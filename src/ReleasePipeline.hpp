#pragma once

#include "Engine.hpp"
#include "webgpu.hpp"
#include "structs.hpp"

void ReleasePipeline(ES::Engine::Core &core)
{
	Pipelines &pipelines = core.GetResource<Pipelines>();

	for (auto &pair : pipelines.renderPipelines) {
		if (pair.second.pipeline != nullptr) {
			pair.second.pipeline.release();
			pair.second.pipeline = nullptr;
		}
	}
}
