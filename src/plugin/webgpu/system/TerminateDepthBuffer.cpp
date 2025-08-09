#include "TerminateDepthBuffer.hpp"
#include "structs.hpp"

namespace ES::Plugin::WebGPU::System {
void TerminateDepthBuffer(ES::Engine::Core &core) {
	if (depthTextureView) {
		depthTextureView.release();
		depthTextureView = nullptr;
	}
}
}
