#pragma once

#include "Engine.hpp"
#include "webgpu.hpp"

void ReleaseUniforms(ES::Engine::Core &core)
{
	if (uniformBuffer) {
		uniformBuffer.release();
		uniformBuffer = nullptr;
	}
}
