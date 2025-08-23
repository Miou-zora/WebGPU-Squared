#include "InitDepthBuffer.hpp"
#include "WebGPU.hpp"
#include "Window.hpp"
#include "structs.hpp"

namespace ES::Plugin::WebGPU::System {

void InitDepthBuffer(ES::Engine::Core &core) {
	auto &device = core.GetResource<wgpu::Device>();
	auto &window = core.GetResource<ES::Plugin::Window::Resource::Window>();
	auto &textureManager = core.GetResource<TextureManager>();

	if (device == nullptr) throw std::runtime_error("WebGPU device is not created, cannot initialize depth buffer.");

	int frameBufferSizeX, frameBufferSizeY;
	glfwGetFramebufferSize(window.GetGLFWWindow(), &frameBufferSizeX, &frameBufferSizeY);

	Texture depthTextureViewData;

	wgpu::TextureDescriptor depthTextureDesc(wgpu::Default);
	depthTextureDesc.label = wgpu::StringView("Z Buffer");
	depthTextureDesc.usage = wgpu::TextureUsage::RenderAttachment;
	depthTextureDesc.size = { static_cast<uint32_t>(frameBufferSizeX), static_cast<uint32_t>(frameBufferSizeY), 1u };
	depthTextureDesc.format = depthTextureFormat;

	depthTextureViewData.texture = device.createTexture(depthTextureDesc);
	depthTextureViewData.textureView = depthTextureViewData.texture.createView();

	if (textureManager.Contains("WindowDepthTexture")) {
		Texture &depthTexture = textureManager.Get("WindowDepthTexture");
		depthTexture.texture = depthTextureViewData.texture;
		depthTexture.textureView = depthTextureViewData.textureView;
	} else {
		textureManager.Add("WindowDepthTexture", depthTextureViewData);
	}
}
}
