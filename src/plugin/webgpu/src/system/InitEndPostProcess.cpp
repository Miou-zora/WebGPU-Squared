#include "InitEndPostProcess.hpp"
#include "structs.hpp"
#include "resource/window/Window.hpp"
#include "plugin/PluginWindow.hpp"
#include <GLFW/glfw3.h>

namespace ES::Plugin::WebGPU::System {

static void InitInputEndProcessTexture(ES::Engine::Core &core)
{
    auto &device = core.GetResource<wgpu::Device>();
    auto &pipelineData = core.GetResource<Pipelines>().renderPipelines["EndPostProcess"];
    auto &textureManager = core.GetResource<TextureManager>();
    auto &window = core.GetResource<ES::Plugin::Window::Resource::Window>();
    auto &bindGroups = core.GetResource<BindGroups>();

    int frameBufferSizeX, frameBufferSizeY;
    glfwGetFramebufferSize(window.GetGLFWWindow(), &frameBufferSizeX, &frameBufferSizeY);

    auto &texture = textureManager.Add("InputEndPostProcess");

    wgpu::TextureDescriptor textureDesc(wgpu::Default);
    textureDesc.label = wgpu::StringView("InputEndPostProcess Texture");
    textureDesc.size = { static_cast<uint32_t>(frameBufferSizeX), static_cast<uint32_t>(frameBufferSizeY), 1 };
    textureDesc.format = wgpu::TextureFormat::RGBA16Float;
    textureDesc.usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::RenderAttachment;

    texture.texture = device.createTexture(textureDesc);
    texture.textureView = texture.texture.createView();

    wgpu::BindGroupEntry binding(wgpu::Default);
	binding.binding = 0;
	binding.textureView = texture.textureView;

    std::array<wgpu::BindGroupEntry, 1> bindings = { binding };

    wgpu::BindGroupDescriptor bindGroupDesc(wgpu::Default);
	bindGroupDesc.layout = pipelineData.bindGroupLayouts[0];
	bindGroupDesc.entryCount = bindings.size();
	bindGroupDesc.entries = bindings.data();
	bindGroupDesc.label = wgpu::StringView("InputEndPostProcess Binding Group Textures");
	auto bg = device.createBindGroup(bindGroupDesc);

	if (bg == nullptr) throw std::runtime_error("Could not create WebGPU bind group");

	bindGroups.groups["InputEndPostProcess"] = bg;
}

void InitEndPostProcess(ES::Engine::Core &core) {
    InitInputEndProcessTexture(core);

    core.GetResource<WindowResizeCallbacks>().callbacks.push_back([](ES::Engine::Core &core, int width, int height) {
        auto &textureManager = core.GetResource<TextureManager>();
        auto &device = core.GetResource<wgpu::Device>();
        auto &bindGroups = core.GetResource<BindGroups>();

		{
			auto &textureNormal = textureManager.Get("InputEndPostProcess");
			textureNormal.textureView.release();
			textureNormal.texture.destroy();
			textureNormal.texture.release();

            bindGroups.groups["InputEndPostProcess"].release();
		}

        textureManager.Remove("InputEndPostProcess");

		InitInputEndProcessTexture(core);
    });
}
}
