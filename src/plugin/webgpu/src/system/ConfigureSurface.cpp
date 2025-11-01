
#include "ConfigureSurface.hpp"
#include "WebGPU.hpp"
#include "resource/Window/Window.hpp"

void ES::Plugin::WebGPU::System::ConfigureSurface(ES::Engine::Core &core) {

	const auto &device = core.GetResource<wgpu::Device>();
	const auto &surface = core.GetResource<wgpu::Surface>();
	const auto &capabilities = core.GetResource<wgpu::SurfaceCapabilities>();
	auto &window = core.GetResource<ES::Plugin::Window::Resource::Window>();

	if (surface == nullptr) throw std::runtime_error("Surface is not created, cannot configure it.");
	if (device == nullptr) throw std::runtime_error("Device is not created, cannot configure surface.");

	int frameBufferSizeX, frameBufferSizeY;
	glfwGetFramebufferSize(window.GetGLFWWindow(), &frameBufferSizeX, &frameBufferSizeY);
	wgpu::SurfaceConfiguration config(wgpu::Default);
	config.width = frameBufferSizeX;
	config.height = frameBufferSizeY;
	config.usage = wgpu::TextureUsage::RenderAttachment;
	config.format = capabilities.formats[0];
	config.viewFormatCount = 0;
	config.viewFormats = nullptr;
	config.device = device;
	config.presentMode = wgpu::PresentMode::Fifo;
	config.alphaMode = wgpu::CompositeAlphaMode::Auto;

	surface.configure(config);
}
