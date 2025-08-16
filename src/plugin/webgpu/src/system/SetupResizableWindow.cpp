#include "WebGPU.hpp"
#include "Window.hpp"
#include "ConfigureSurface.hpp"
#include "InitDepthBuffer.hpp"
#include "UnconfigureSurface.hpp"
#include "TerminateDepthBuffer.hpp"
#include "structs.hpp"

namespace ES::Plugin::WebGPU::System {

static void onResize(GLFWwindow* window, int width, int height) {
	auto corePtr = static_cast<ES::Engine::Core *>(glfwGetWindowUserPointer(window));

	if (!corePtr) throw std::runtime_error("Window user pointer is null, cannot resize.");

	auto &core = *corePtr;
	auto &callbacks = core.GetResource<WindowResizeCallbacks>();

	for (auto &[name, callback] : callbacks.callbacks) {
		// TODO: should be have a try-catch here or just a basic call ?
		try {
			callback(core, width, height);
		} catch (const std::exception &e) {
			ES::Utils::Log::Error(fmt::format("Error in resize callback '{}': {}", name, e.what()));
		}
	}
}

static void updateSurface(ES::Engine::Core &core, int width, int height) {
	TerminateDepthBuffer(core);
	UnconfigureSurface(core);

	ES::Plugin::WebGPU::System::ConfigureSurface(core);
	ES::Plugin::WebGPU::System::InitDepthBuffer(core);

	auto &cameraData = core.GetResource<CameraData>();
	cameraData.aspectRatio = static_cast<float>(width) / static_cast<float>(height);
}

void SetupResizableWindow(ES::Engine::Core &core) {
	auto &windowResizeCallbacks = core.RegisterResource(WindowResizeCallbacks());
	core.GetResource<ES::Plugin::Window::Resource::Window>().SetResizable(true);
	core.GetResource<ES::Plugin::Window::Resource::Window>().SetFramebufferSizeCallback(&core, onResize);

	windowResizeCallbacks.callbacks["updateSurface"] = updateSurface;
}

}
