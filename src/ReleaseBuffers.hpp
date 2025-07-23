#pragma once

#include "Mesh.hpp"

void ReleaseBuffers(ES::Engine::Core &core)
{
	core.GetRegistry().view<Mesh>().each([](Mesh &mesh) {
		mesh.Release();
	});
}
