#include <imgui.h>
#include <backends/imgui_impl_wgpu.h>
#include <backends/imgui_impl_glfw.h>

#include "PluginImGUI.hpp"
#include "WebGPU.hpp"
#include "RenderingPipeline.hpp"
#include "resource/window/Window.hpp"
#include "RenderGUI.hpp"
#include "PipelineType.hpp"

namespace ES::Plugin::ImGUI {
    namespace WebGPU {
        void Plugin::Bind() {
		RequirePlugins<ES::Plugin::WebGPU::Plugin>();

		RegisterSystems<ES::Plugin::RenderingPipeline::Setup>(
			[](ES::Engine::Core &core) {
				auto &window = core.GetResource<ES::Plugin::Window::Resource::Window>();
				IMGUI_CHECKVERSION();
				ImGui::CreateContext();
				ImGuiIO& io = ImGui::GetIO();

				io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
				io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

				ImGui_ImplGlfw_InitForOther(window.GetGLFWWindow(), true);
			},
			[](ES::Engine::Core &core) {
				ImGui_ImplWGPU_InitInfo info = ImGui_ImplWGPU_InitInfo();
				info.DepthStencilFormat = depthTextureFormat;
				info.RenderTargetFormat = core.GetResource<wgpu::SurfaceCapabilities>().formats[0];
				info.Device = core.GetResource<wgpu::Device>();
				ImGui_ImplWGPU_Init(&info);
			},
			[](ES::Engine::Core &core) {
				auto &renderGraph = core.GetResource<RenderGraph>();
				renderGraph.AddRenderPass(
					RenderPassData{
						.name = "GUIRenderPass",
						.pipelineType = PipelineType::None, // Custom one
						.loadOp = wgpu::LoadOp::Load,
						.outputColorTextureName = {"WindowColorTexture"},
						.outputDepthTextureName = "WindowDepthTexture",
						.uniqueRenderCallback = Util::RenderGUI
					}
				);
			}
		);
	}
    }
}
