#include "ReleaseBuffers.hpp"
#include "Mesh.hpp"

namespace ES::Plugin::WebGPU::System {

void ReleaseBuffers(ES::Engine::Core &core)
{
	core.GetRegistry().view<ES::Plugin::WebGPU::Component::Mesh>().each([](ES::Plugin::WebGPU::Component::Mesh &mesh) {
		mesh.Release();
	});
}
}
