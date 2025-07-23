#pragma once

#include "Engine.hpp"
#include "webgpu.hpp"

void ReleasePipeline(ES::Engine::Core &core)
{
	wgpu::RenderPipeline &pipeline = core.GetResource<wgpu::RenderPipeline>();

	if (pipeline) {
		pipeline.release();
		pipeline = nullptr;
	}
}
