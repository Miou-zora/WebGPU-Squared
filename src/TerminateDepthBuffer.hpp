#pragma once

#include "webgpu.hpp"
#include "Engine.hpp"

void TerminateDepthBuffer(ES::Engine::Core &core) {
	Pipelines &pipelines = core.GetResource<Pipelines>();

	for (auto &pair : pipelines.renderPipelines) {
		if (pair.second.depthTextureView) {
			pair.second.depthTextureView.release();
			pair.second.depthTextureView = nullptr;
		}
	}
}
