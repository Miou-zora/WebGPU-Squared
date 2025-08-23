#include "GetNextSurfaceViewData.hpp"
#include "structs.hpp"

namespace ES::Plugin::WebGPU::Util {

void GetNextSurfaceViewData(ES::Engine::Core &core, wgpu::Surface &surface){
	wgpu::SurfaceTexture surfaceTexture(wgpu::Default);
	surface.getCurrentTexture(&surfaceTexture);
	if (
	    surfaceTexture.status != wgpu::SurfaceGetCurrentTextureStatus::SuccessOptimal && // NEW
	    surfaceTexture.status != wgpu::SurfaceGetCurrentTextureStatus::SuccessSuboptimal
	) {
	    throw std::runtime_error("Failed to get current surface texture");
	}
	wgpu::Texture texturewgpu = surfaceTexture.texture;

	// Create a view for this surface texture
	wgpu::TextureViewDescriptor viewDescriptor(wgpu::Default);
	viewDescriptor.label = wgpu::StringView("Surface texture view");
	viewDescriptor.format = texturewgpu.getFormat();
	viewDescriptor.dimension = wgpu::TextureViewDimension::_2D;
	viewDescriptor.mipLevelCount = 1;
	viewDescriptor.arrayLayerCount = 1;
	wgpu::TextureView targetView = texturewgpu.createView(viewDescriptor);

	if (targetView == nullptr) throw std::runtime_error("Could not get next surface texture view");

	if (core.GetResource<TextureManager>().Contains("WindowColorTexture")) {
		Texture &texture = core.GetResource<TextureManager>().Get("WindowColorTexture");
		texture.textureView = targetView;
		texture.texture = texturewgpu;
	} else {
		Texture texture;
		texture.textureView = targetView;
		texture.texture = texturewgpu;
		core.GetResource<TextureManager>().Add("WindowColorTexture", texture);
	}
}
}
