#pragma once

#include "webgpu.hpp"
#include "Engine.hpp"

void TerminateDepthBuffer(ES::Engine::Core &core) {
	if (depthTextureView) {
		depthTextureView.release();
		depthTextureView = nullptr;
	}
}
