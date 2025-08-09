#pragma once

#include "webgpu.hpp"
#include "PluginWindow.hpp"
#include "ConfigureSurface.hpp"
#include "InitDepthBuffer.hpp"
#include "UnconfigureSurface.hpp"
#include "TerminateDepthBuffer.hpp"

void onResize(GLFWwindow* window, int width, int height) {
	auto core = reinterpret_cast<ES::Engine::Core*>(glfwGetWindowUserPointer(window));

    if (core == nullptr) throw std::runtime_error("Window user pointer is null, cannot resize.");

	TerminateDepthBuffer(*core);
	UnconfigureSurface(*core);

	ES::Plugin::WebGPU::System::ConfigureSurface(*core);
	ES::Plugin::WebGPU::System::InitDepthBuffer(*core);

	auto &cameraData = core->GetResource<CameraData>();
	cameraData.aspectRatio = static_cast<float>(width) / static_cast<float>(height);
}

void SetupResizableWindow(ES::Engine::Core &core) {
	core.GetResource<ES::Plugin::Window::Resource::Window>().SetResizable(true);
	core.GetResource<ES::Plugin::Window::Resource::Window>().SetFramebufferSizeCallback(&core, onResize);
}
