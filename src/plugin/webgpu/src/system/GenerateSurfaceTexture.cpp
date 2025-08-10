#include "GenerateSurfaceTexture.hpp"

#include "GetNextSurfaceViewData.hpp"
#include "structs.hpp"

namespace ES::Plugin::WebGPU::System {
void GenerateSurfaceTexture(ES::Engine::Core &core)
{
	wgpu::Surface &surface = core.GetResource<wgpu::Surface>();
	textureView = ES::Plugin::WebGPU::Util::GetNextSurfaceViewData(surface);
	if (textureView == nullptr) throw std::runtime_error("Could not get next surface texture view");

	Texture texture;

	texture.textureView = textureView;

	if (core.GetResource<TextureManager>().Contains("WindowColorTexture")) {
		core.GetResource<TextureManager>().Get("WindowColorTexture").textureView = textureView;
	} else {
		core.GetResource<TextureManager>().Add("WindowColorTexture", texture);
	}
}
}
