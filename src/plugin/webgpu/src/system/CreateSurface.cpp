#include "CreateSurface.hpp"
#include "WebGPU.hpp"
#include "resource/window/Window.hpp"
#include <glfw3webgpu.h>

namespace ES::Plugin::WebGPU::System {

void CreateSurface(ES::Engine::Core &core) {
	ES::Utils::Log::Debug("Creating surface...");

	auto &instance = core.GetResource<wgpu::Instance>();
	auto glfwWindow = core.GetResource<ES::Plugin::Window::Resource::Window>().GetGLFWWindow();

	wgpu::Surface &surface = core.RegisterResource(wgpu::Surface(glfwCreateWindowWGPUSurface(instance, glfwWindow)));

	glfwMakeContextCurrent(glfwWindow);

	if (surface == nullptr) throw std::runtime_error("Could not create surface");

	ES::Utils::Log::Debug(fmt::format("Surface created: {}", static_cast<void*>(surface)));
}

}
