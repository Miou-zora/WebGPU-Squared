#include "GetNextSurfaceViewData.hpp"
#include "structs.hpp"

namespace ES::Plugin::WebGPU::Util {

wgpu::TextureView GetNextSurfaceViewData(wgpu::Surface &surface){
	wgpu::SurfaceTexture surfaceTexture(wgpu::Default);
	surface.getCurrentTexture(&surfaceTexture);
	if (
	    surfaceTexture.status != wgpu::SurfaceGetCurrentTextureStatus::SuccessOptimal && // NEW
	    surfaceTexture.status != wgpu::SurfaceGetCurrentTextureStatus::SuccessSuboptimal
	) {
	    return nullptr;
	}
	wgpu::Texture texture = surfaceTexture.texture;

	// Create a view for this surface texture
	wgpu::TextureViewDescriptor viewDescriptor(wgpu::Default);
	viewDescriptor.label = wgpu::StringView("Surface texture view");
	viewDescriptor.format = texture.getFormat();
	viewDescriptor.dimension = wgpu::TextureViewDimension::_2D;
	viewDescriptor.mipLevelCount = 1;
	viewDescriptor.arrayLayerCount = 1;
	wgpu::TextureView targetView = texture.createView(viewDescriptor);

	textureToRelease = texture;

	return targetView;
}
}
