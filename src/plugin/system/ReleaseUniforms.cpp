#include "ReleaseUniforms.hpp"
#include "structs.hpp"

namespace ES::Plugin::WebGPU::System {
void ReleaseUniforms(ES::Engine::Core &core)
{
	if (uniformBuffer) {
		uniformBuffer.release();
		uniformBuffer = nullptr;
	}
}
}
