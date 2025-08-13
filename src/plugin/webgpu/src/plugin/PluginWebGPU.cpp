#include "PluginWindow.hpp"
#include "RenderingPipeline.hpp"
#include "Entity.hpp"

#include "WebGPU.hpp"

namespace ES::Plugin::WebGPU {
void Plugin::Bind() {
    RequirePlugins<ES::Plugin::Window::Plugin>();

    RegisterResource(ClearColor());
    RegisterResource(Pipelines());
    RegisterResource(TextureManager());
    RegisterResource(std::vector<Light>());
    RegisterResource(CameraData());
    RegisterResource(RenderGraph());

    RegisterSystems<ES::Plugin::RenderingPipeline::Setup>(
        System::CreateInstance,
        System::CreateSurface,
        System::CreateAdapter,
#if defined(ES_DEBUG)
        System::AdaptaterPrintLimits,
        System::AdaptaterPrintFeatures,
        System::AdaptaterPrintProperties,
#endif
        System::ReleaseInstance,
        System::RequestCapabilities,
        System::CreateDevice,
        System::CreateQueue,
        System::SetupQueueOnSubmittedWorkDone,
        System::ConfigureSurface,
        System::ReleaseAdapter,
#if defined(ES_DEBUG)
        System::InspectDevice,
#endif
        System::InitDepthBuffer,
        System::InitializePipeline,
        System::Initialize2DPipeline,
        System::InitializeBuffers,
        System::Create2DPipelineBuffer,
        System::CreateBindingGroup,
        System::CreateBindingGroup2D,
        System::SetupResizableWindow,
        [](ES::Engine::Core &core) {
            stbi_set_flip_vertically_on_load(true);
        },
        [](ES::Engine::Core &core) {
            core.GetResource<RenderGraph>().AddRenderPass(
                RenderPassData{
                    .name = "ClearRenderPass",
                    .outputColorTextureName = "WindowColorTexture",
                    .outputDepthTextureName = "WindowDepthTexture",
                    .loadOp = wgpu::LoadOp::Clear,
                .clearColor = [](ES::Engine::Core &core) -> glm::vec4 {
                    auto &clearColor = core.GetResource<ClearColor>().value;
                    return glm::vec4(
                        clearColor.r,
                        clearColor.g,
                        clearColor.b,
                        clearColor.a
                    );
                },
            });
            core.GetResource<RenderGraph>().AddRenderPass(
                RenderPassData{
                    .name = "3DRenderPass",
                    .outputColorTextureName = "WindowColorTexture",
                    .outputDepthTextureName = "WindowDepthTexture",
                .loadOp = wgpu::LoadOp::Load,
                .bindGroups = {
                    { .groupIndex = 0, .type = BindGroupsLinks::AssetType::BindGroup, .name = "1" },
                    { .groupIndex = 1, .type = BindGroupsLinks::AssetType::BindGroup, .name = "2" }
                },
                .pipelineName = "3D"
            });
            core.GetResource<RenderGraph>().AddRenderPass(
                RenderPassData{
                    .name = "2DRenderPass",
                    .outputColorTextureName = "WindowColorTexture",
                    .outputDepthTextureName = "WindowDepthTexture",
                    .loadOp = wgpu::LoadOp::Load,
                .bindGroups = {
                    { .groupIndex = 0, .type = BindGroupsLinks::AssetType::BindGroup, .name = "2D" },
                },
                .pipelineName = "2D",
                .perEntityCallback = [](wgpu::RenderPassEncoder &renderPass, ES::Engine::Core &core, ES::Plugin::WebGPU::Component::Mesh &mesh, ES::Plugin::Object::Component::Transform &transform, ES::Engine::Entity entity) {
                    auto &textures = core.GetResource<TextureManager>();
                    auto texture = textures.Get(mesh.textures[0]);
                    renderPass.setBindGroup(1, texture.bindGroup, 0, nullptr);
                }
            });
        }
    );
    RegisterSystems<ES::Plugin::RenderingPipeline::ToGPU>(
        System::UpdateBuffers,
        System::GenerateSurfaceTexture,
        [](ES::Engine::Core &core) {
            core.GetResource<RenderGraph>().Execute(core);
        }
    );
    RegisterSystems<ES::Plugin::RenderingPipeline::Draw>(
        System::Render
    );
    RegisterSystems<ES::Engine::Scheduler::Shutdown>(
        System::ReleaseBindingGroup,
        System::ReleaseUniforms,
        System::ReleaseBuffers,
        System::TerminateDepthBuffer,
        System::ReleasePipeline,
        System::ReleaseDevice,
        System::ReleaseSurface,
        System::ReleaseQueue
    );
}
}
