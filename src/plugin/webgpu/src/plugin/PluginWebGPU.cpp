#include "PluginWindow.hpp"
#include "RenderingPipeline.hpp"
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
        System::InitializeDeferredPipeline,
        System::InitializeBuffers,
        System::Create2DPipelineBuffer,
        System::CreateBindingGroup,
        System::CreateBindingGroup2D,
        System::SetupResizableWindow,
        System::GenerateDefaultTexture,
        [](ES::Engine::Core &core) { stbi_set_flip_vertically_on_load(true); },
        System::InitGBufferTextures,
        System::InitializeGBufferPipeline,
        System::InitGBufferBuffers,
        System::CreateBindingGroupGBuffer,
        System::CreateBindingGroupDeferred,
        [](ES::Engine::Core &core) {
            core.GetResource<RenderGraph>().AddRenderPass(
                RenderPassData{
                    .name = "GBuffer",
                    .shaderName = "GBuffer",
                    .pipelineType = PipelineType::_3D,
                    .loadOp = wgpu::LoadOp::Clear,
                    .clearColor = [](ES::Engine::Core &core) -> glm::vec4 {
                        return glm::vec4(0, 0, 0, 0);
                    },
                    .outputColorTextureName = {"gBufferTexture2DFloat16", "gBufferTextureAlbedo"},
                    .outputDepthTextureName = "depthTexture",
                    .bindGroups = {
                        { .groupIndex = 0, .type = BindGroupsLinks::AssetType::BindGroup, .name = "GBuffer" },
                        { .groupIndex = 1, .type = BindGroupsLinks::AssetType::BindGroup, .name = "2D" },
                    },
                    .perEntityCallback = [](wgpu::RenderPassEncoder &renderPass, ES::Engine::Core &core, ES::Plugin::WebGPU::Component::Mesh &mesh, ES::Plugin::Object::Component::Transform &transform, ES::Engine::Entity entity) {
                        wgpu::Queue queue = core.GetResource<wgpu::Queue>();

                        Uniforms uniforms;
                        uniforms.modelMatrix = transform.getTransformationMatrix();
                        uniforms.normalModelMatrix = glm::transpose(glm::inverse(uniforms.modelMatrix));

                        queue.writeBuffer(uniformsBuffer, 0, &uniforms, sizeof(uniforms));

                        auto &textures = core.GetResource<TextureManager>();
                        entt::hashed_string textureName = entt::hashed_string("DEFAULT_TEXTURE");
                        if (mesh.textures.size() > 0 && textures.Contains(mesh.textures[0])) {
                            textureName = mesh.textures[0];
                        }
                        auto texture = textures.Get(textureName);
                        renderPass.setBindGroup(1, texture.bindGroup, 0, nullptr);
                    }
                }
            );
            core.GetResource<RenderGraph>().AddRenderPass(
                RenderPassData{
                    .name = "Deferred",
                    .shaderName = "Deferred",
                    .pipelineType = PipelineType::_3D,
                    .loadOp = wgpu::LoadOp::Clear,
                    .clearColor = [](ES::Engine::Core &core) -> glm::vec4 {
                        auto &clearColor = core.GetResource<ClearColor>().value;
                        return glm::vec4(0.1, 0.2, 0.3, 1.0);
                    },
                    .outputColorTextureName = {"WindowColorTexture"},
                    .outputDepthTextureName = "WindowDepthTexture",
                    .bindGroups = {
                        { .groupIndex = 0, .type = BindGroupsLinks::AssetType::BindGroup, .name = "DeferredGroup0" },
                        { .groupIndex = 1, .type = BindGroupsLinks::AssetType::BindGroup, .name = "2" },
                        { .groupIndex = 2, .type = BindGroupsLinks::AssetType::BindGroup, .name = "DeferredGroup2" }
                    },
                    .uniqueRenderCallback = [](wgpu::RenderPassEncoder &renderPass, ES::Engine::Core &core) {
                        renderPass.draw(6, 1, 0, 0);
                    }
                }
            );
            core.GetResource<RenderGraph>().AddRenderPass(
                RenderPassData{
                    .name = "2D",
                    .shaderName = "2D",
                    .pipelineType = PipelineType::_2D,
                    .loadOp = wgpu::LoadOp::Load,
                    .outputColorTextureName = {"WindowColorTexture"},
                    .outputDepthTextureName = "WindowDepthTexture",
                    .bindGroups = {
                        { .groupIndex = 0, .type = BindGroupsLinks::AssetType::BindGroup, .name = "2D" },
                    },
                    .perEntityCallback = [](wgpu::RenderPassEncoder &renderPass, ES::Engine::Core &core, ES::Plugin::WebGPU::Component::Mesh &mesh, ES::Plugin::Object::Component::Transform &transform, ES::Engine::Entity entity) {
                        auto &textures = core.GetResource<TextureManager>();
                        entt::hashed_string textureName = entt::hashed_string("DEFAULT_TEXTURE");
                        if (mesh.textures.size() > 0 && textures.Contains(mesh.textures[0])) {
                            textureName = mesh.textures[0];
                        }
                        auto texture = textures.Get(textureName);
                        renderPass.setBindGroup(1, texture.bindGroup, 0, nullptr);
                    }
                }
            );
        }
    );
    RegisterSystems<ES::Plugin::RenderingPipeline::ToGPU>(
        System::UpdateBuffers,
        System::UpdateCameraBuffer,
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
