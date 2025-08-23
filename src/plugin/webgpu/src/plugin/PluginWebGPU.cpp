#include "PluginWindow.hpp"
#include "RenderingPipeline.hpp"
#include "Entity.hpp"
#include "Window.hpp"
#include "WebGPU.hpp"

struct Uniforms {
    glm::mat4 modelMatrix;
    glm::mat4 normalModelMatrix;
};

struct Camera {
    glm::mat4 viewProjectionMatrix;
    glm::mat4 invViewProjectionMatrix;
    glm::vec3 position;
    float _padding;
};

wgpu::Buffer uniformsBuffer;

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
        [](ES::Engine::Core &core) {
            auto &textureManager = core.GetResource<TextureManager>();
			auto &pipelines = core.GetResource<Pipelines>();
			textureManager.Add(entt::hashed_string("DEFAULT_TEXTURE"), core.GetResource<wgpu::Device>(), glm::uvec2(2, 2), [](glm::uvec2 pos) {
				glm::u8vec4 color;
                // Checkerboard pattern of pink
				color.r = ((pos.x + pos.y) % 2 == 0) ? 255 : 0;
				color.g = 0;
				color.b = ((pos.x + pos.y) % 2 == 0) ? 255 : 0;
				color.a = 255;
				return color;
			}, pipelines.renderPipelines["2D"].bindGroupLayouts[1]);
        },
        [](ES::Engine::Core &core) {
            stbi_set_flip_vertically_on_load(true);
        },
        [](ES::Engine::Core &core) {
            wgpu::Device device = core.GetResource<wgpu::Device>();
			auto &pipelines = core.GetResource<Pipelines>();
            auto &textureManager = core.GetResource<TextureManager>();
            auto &window = core.GetResource<ES::Plugin::Window::Resource::Window>();

            int frameBufferSizeX, frameBufferSizeY;
            glfwGetFramebufferSize(window.GetGLFWWindow(), &frameBufferSizeX, &frameBufferSizeY);

            auto &textureNormal = textureManager.Add("gBufferTexture2DFloat16");

            wgpu::TextureDescriptor textureDesc(wgpu::Default);
            textureDesc.label = wgpu::StringView("gBufferTexture2DFloat16");
            textureDesc.size = { static_cast<uint32_t>(frameBufferSizeX), static_cast<uint32_t>(frameBufferSizeY), 1 };
            textureDesc.format = wgpu::TextureFormat::RGBA16Float;
            textureDesc.usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::RenderAttachment;

            textureNormal.texture = device.createTexture(textureDesc);
            textureNormal.textureView = textureNormal.texture.createView();

            core.GetResource<WindowResizeCallbacks>().callbacks.push_back([](ES::Engine::Core &core, int width, int height) {
                auto &textureManager = core.GetResource<TextureManager>();
                auto &pipelines = core.GetResource<Pipelines>();
                auto &device = core.GetResource<wgpu::Device>();

                textureManager.Remove("gBufferTexture2DFloat16");

                auto &textureNormal = textureManager.Add("gBufferTexture2DFloat16");

                wgpu::TextureDescriptor textureDesc(wgpu::Default);
                textureDesc.label = wgpu::StringView("gBufferTexture2DFloat16");
                textureDesc.size = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
                textureDesc.format = wgpu::TextureFormat::RGBA16Float;
                textureDesc.usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::RenderAttachment;

                textureNormal.texture = device.createTexture(textureDesc);
                textureNormal.textureView = textureNormal.texture.createView();
            });
            Texture &textureAlbedo = core.GetResource<TextureManager>().Add("gBufferTextureAlbedo");

            textureDesc.label = wgpu::StringView("gBufferTextureAlbedo");
            textureDesc.size = { static_cast<uint32_t>(frameBufferSizeX), static_cast<uint32_t>(frameBufferSizeY), 1 };
            textureDesc.format = wgpu::TextureFormat::BGRA8Unorm;
            textureDesc.usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::RenderAttachment;

            textureAlbedo.texture = device.createTexture(textureDesc);
            textureAlbedo.textureView = textureAlbedo.texture.createView();

            core.GetResource<WindowResizeCallbacks>().callbacks.push_back([](ES::Engine::Core &core, int width, int height) {
                auto &textureManager = core.GetResource<TextureManager>();
                auto &pipelines = core.GetResource<Pipelines>();
                auto &device = core.GetResource<wgpu::Device>();

                textureManager.Remove("gBufferTextureAlbedo");

                auto &textureAlbedo = textureManager.Add("gBufferTextureAlbedo");

                wgpu::TextureDescriptor textureDesc(wgpu::Default);
                textureDesc.label = wgpu::StringView("gBufferTextureAlbedo");
                textureDesc.size = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
                textureDesc.format = wgpu::TextureFormat::BGRA8Unorm;
                textureDesc.usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::RenderAttachment;

                auto gBufferTextureAlbedoTexture = device.createTexture(textureDesc);
                auto gBufferTextureAlbedoTextureView = gBufferTextureAlbedoTexture.createView();

                textureAlbedo.texture = gBufferTextureAlbedoTexture;
                textureAlbedo.textureView = gBufferTextureAlbedoTextureView;
            });

            Texture &depthTexture = core.GetResource<TextureManager>().Add("depthTexture");

            textureDesc.label = wgpu::StringView("depthTexture");
            textureDesc.size = { static_cast<uint32_t>(frameBufferSizeX), static_cast<uint32_t>(frameBufferSizeY), 1 };
            textureDesc.format = wgpu::TextureFormat::Depth24Plus;
            textureDesc.usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::RenderAttachment;

            depthTexture.texture = device.createTexture(textureDesc);
            depthTexture.textureView = depthTexture.texture.createView();

            core.GetResource<WindowResizeCallbacks>().callbacks.push_back([&depthTexture](ES::Engine::Core &core, int width, int height) {
                auto &textureManager = core.GetResource<TextureManager>();
                auto &pipelines = core.GetResource<Pipelines>();
                auto &device = core.GetResource<wgpu::Device>();

                textureManager.Remove("depthTexture");

                auto &depthTexture = textureManager.Add("depthTexture");

                wgpu::TextureDescriptor textureDesc(wgpu::Default);
                textureDesc.label = wgpu::StringView("depthTexture");
                textureDesc.size = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
                textureDesc.format = wgpu::TextureFormat::Depth24Plus;
                textureDesc.usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::RenderAttachment;

                depthTexture.texture = device.createTexture(textureDesc);
                depthTexture.textureView = depthTexture.texture.createView();
            });
        },
        [](ES::Engine::Core &core) {
            auto &device = core.GetResource<wgpu::Device>();
            auto &surface = core.GetResource<wgpu::Surface>();
            auto &window = core.GetResource<ES::Plugin::Window::Resource::Window>();

            if (device == nullptr) throw std::runtime_error("WebGPU device is not created, cannot initialize 2D pipeline.");
            if (surface == nullptr) throw std::runtime_error("WebGPU surface is not created, cannot initialize 2D pipeline.");

            const std::string pipelineName = "GBuffer";

            wgpu::RenderPipelineDescriptor pipelineDesc(wgpu::Default);
            pipelineDesc.label = wgpu::StringView(fmt::format("{} Pipeline", pipelineName));

            wgpu::ShaderSourceWGSL wgslDesc(wgpu::Default);
            std::string wgslSource = loadFile("shaderGBuffer.wgsl");
            wgslDesc.code = wgpu::StringView(wgslSource);
            wgpu::ShaderModuleDescriptor shaderDesc(wgpu::Default);
            shaderDesc.nextInChain = &wgslDesc.chain;
            shaderDesc.label = wgpu::StringView("Shader source GBuffer");
            wgpu::ShaderModule shaderModule = device.createShaderModule(shaderDesc);
            wgpu::VertexBufferLayout vertexBufferLayout(wgpu::Default);

            std::vector<wgpu::VertexAttribute> vertexAttribs(3);

            // Describe the position attribute
            vertexAttribs[0].shaderLocation = 0;
            vertexAttribs[0].offset = 0;
            vertexAttribs[0].format = wgpu::VertexFormat::Float32x3;

            vertexAttribs[1].shaderLocation = 1;
            vertexAttribs[1].offset = 3 * sizeof(float); // Offset by the size of the position attribute
            vertexAttribs[1].format = wgpu::VertexFormat::Float32x3;

            vertexAttribs[2].shaderLocation = 2;
            vertexAttribs[2].offset = 6 * sizeof(float); // Offset by the size of the position attribute
            vertexAttribs[2].format = wgpu::VertexFormat::Float32x2;

            vertexBufferLayout.attributeCount = static_cast<uint32_t>(vertexAttribs.size());
            vertexBufferLayout.attributes = vertexAttribs.data();

            vertexBufferLayout.arrayStride = (8 * sizeof(float));
            vertexBufferLayout.stepMode = wgpu::VertexStepMode::Vertex;

            // TODO: find why it does not work with wgpu::BindGroupLayoutEntry
            WGPUBindGroupLayoutEntry bindingLayout = {0};
            bindingLayout.binding = 0;
            bindingLayout.visibility = wgpu::ShaderStage::Vertex;
            bindingLayout.buffer.type = wgpu::BufferBindingType::Uniform;
            bindingLayout.buffer.minBindingSize = sizeof(glm::mat4) * 2;

            WGPUBindGroupLayoutEntry bindingLayoutCamera = {0};
            bindingLayoutCamera.binding = 1;
            bindingLayoutCamera.visibility = wgpu::ShaderStage::Vertex;
            bindingLayoutCamera.buffer.type = wgpu::BufferBindingType::Uniform;
            bindingLayoutCamera.buffer.minBindingSize = sizeof(glm::mat4) * 2 + sizeof(glm::vec3) + sizeof(float);

            std::array<WGPUBindGroupLayoutEntry, 2> uniformsBindings = { bindingLayout, bindingLayoutCamera };

            wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc(wgpu::Default);
            bindGroupLayoutDesc.entryCount = uniformsBindings.size();
            bindGroupLayoutDesc.entries = uniformsBindings.data();
            bindGroupLayoutDesc.label = wgpu::StringView("Uniforms Bind Group Layout");
            wgpu::BindGroupLayout uniformsBindGroupLayout = device.createBindGroupLayout(bindGroupLayoutDesc);

            WGPUBindGroupLayoutEntry textureBindingLayout = {0};
            textureBindingLayout.binding = 0;
            textureBindingLayout.visibility = wgpu::ShaderStage::Fragment;
            textureBindingLayout.texture.sampleType = wgpu::TextureSampleType::Float;
            textureBindingLayout.texture.viewDimension = wgpu::TextureViewDimension::_2D;

            WGPUBindGroupLayoutEntry samplerBindingLayout = {0};
            samplerBindingLayout.binding = 1;
            samplerBindingLayout.visibility = wgpu::ShaderStage::Fragment;
            samplerBindingLayout.sampler.type = wgpu::SamplerBindingType::Filtering;

            std::array<WGPUBindGroupLayoutEntry, 2> textureBindings = { textureBindingLayout, samplerBindingLayout };

            bindGroupLayoutDesc.entryCount = textureBindings.size();
            bindGroupLayoutDesc.entries = textureBindings.data();
            bindGroupLayoutDesc.label = wgpu::StringView("Texture Bind Group Layout");
            wgpu::BindGroupLayout textureBindGroupLayout = device.createBindGroupLayout(bindGroupLayoutDesc);

            std::array<WGPUBindGroupLayout, 2> bindGroupLayouts = { uniformsBindGroupLayout, textureBindGroupLayout };

            wgpu::PipelineLayoutDescriptor layoutDesc(wgpu::Default);
            layoutDesc.bindGroupLayoutCount = bindGroupLayouts.size();
            layoutDesc.bindGroupLayouts = bindGroupLayouts.data();
            wgpu::PipelineLayout layout = device.createPipelineLayout(layoutDesc);

            pipelineDesc.vertex.bufferCount = 1;
            pipelineDesc.vertex.buffers = &vertexBufferLayout;
            pipelineDesc.vertex.module = shaderModule;
            pipelineDesc.vertex.entryPoint = wgpu::StringView("vs_main");

            wgpu::FragmentState fragmentState(wgpu::Default);
            fragmentState.module = shaderModule;
            fragmentState.entryPoint = wgpu::StringView("fs_main");

            std::vector<wgpu::ColorTargetState> colorTargets;

            wgpu::ColorTargetState normalColorTarget(wgpu::Default);
            normalColorTarget.format = wgpu::TextureFormat::RGBA16Float;
            normalColorTarget.writeMask = wgpu::ColorWriteMask::All;
            wgpu::BlendState blendState(wgpu::Default);
            normalColorTarget.blend = &blendState;

            wgpu::ColorTargetState albedoColorTarget(wgpu::Default);
            albedoColorTarget.format = wgpu::TextureFormat::BGRA8Unorm;
            albedoColorTarget.writeMask = wgpu::ColorWriteMask::All;
            albedoColorTarget.blend = &blendState;

            colorTargets.push_back(normalColorTarget);
            colorTargets.push_back(albedoColorTarget);

            fragmentState.targetCount = static_cast<uint32_t>(colorTargets.size());
            fragmentState.targets = colorTargets.data();
            pipelineDesc.fragment = &fragmentState;
            pipelineDesc.layout = layout;

            int frameBufferSizeX, frameBufferSizeY;
            glfwGetFramebufferSize(window.GetGLFWWindow(), &frameBufferSizeX, &frameBufferSizeY);

            wgpu::DepthStencilState depthStencilState(wgpu::Default);
            depthStencilState.depthCompare = wgpu::CompareFunction::Less;
            depthStencilState.depthWriteEnabled = wgpu::OptionalBool::True;
            depthStencilState.format = wgpu::TextureFormat::Depth24Plus;
            pipelineDesc.depthStencil = &depthStencilState;

            pipelineDesc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
            pipelineDesc.primitive.cullMode = wgpu::CullMode::Back;

            wgpu::RenderPipeline pipeline = device.createRenderPipeline(pipelineDesc);

            if (pipeline == nullptr) throw std::runtime_error("Could not create render pipeline");

            wgpuShaderModuleRelease(shaderModule);

            core.GetResource<Pipelines>().renderPipelines["GBuffer"] = PipelineData{
                .pipeline = pipeline,
                .bindGroupLayouts = {
                    uniformsBindGroupLayout,
                    textureBindGroupLayout
                },
                .layout = layout,
            };
        },
        [](ES::Engine::Core &core) {
            wgpu::Queue &queue = core.GetResource<wgpu::Queue>();
            wgpu::Device &device = core.GetResource<wgpu::Device>();

            if (queue == nullptr) throw std::runtime_error("WebGPU queue is not created, cannot initialize buffers.");
            if (device == nullptr) throw std::runtime_error("WebGPU device is not created, cannot initialize buffers.");

            wgpu::BufferDescriptor bufferDesc(wgpu::Default);
            bufferDesc.size = sizeof(Uniforms);
            bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
            bufferDesc.label = wgpu::StringView("Uniforms Buffer for GBuffer");
            uniformsBuffer = device.createBuffer(bufferDesc);

            Uniforms uniforms;
            uniforms.modelMatrix = glm::mat4(1.0f);
            uniforms.normalModelMatrix = glm::mat4(1.0f);
            queue.writeBuffer(uniformsBuffer, 0, &uniforms, sizeof(uniforms));

            bufferDesc.size = sizeof(Camera);
            bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
            bufferDesc.label = wgpu::StringView("Camera Buffer for GBuffer");
            cameraBuffer = device.createBuffer(bufferDesc);

            Camera camera;
            camera.viewProjectionMatrix = glm::mat4(1.0f);
            camera.invViewProjectionMatrix = glm::mat4(1.0f);
            queue.writeBuffer(cameraBuffer, 0, &camera, sizeof(camera));
        },
        System::CreateBindingGroupDeferred,
        [](ES::Engine::Core &core) {
            auto &device = core.GetResource<wgpu::Device>();
            auto &pipelineData = core.GetResource<Pipelines>().renderPipelines["GBuffer"];
            auto &bindGroups = core.GetResource<BindGroups>();
            //TODO: Put this in a separate system
            //TODO: Should we separate this from pipelineData?

            if (device == nullptr) throw std::runtime_error("WebGPU device is not created, cannot create binding group.");

            wgpu::BindGroupEntry bindingUniforms(wgpu::Default);
            bindingUniforms.binding = 0;
            bindingUniforms.buffer = uniformsBuffer;
            bindingUniforms.size = sizeof(Uniforms);

            wgpu::BindGroupEntry bindingCamera(wgpu::Default);
            bindingCamera.binding = 1;
            bindingCamera.buffer = cameraBuffer;
            bindingCamera.size = sizeof(Camera);

            std::array<wgpu::BindGroupEntry, 2> bindings = { bindingUniforms, bindingCamera };

            wgpu::BindGroupDescriptor bindGroupDesc(wgpu::Default);
            bindGroupDesc.layout = pipelineData.bindGroupLayouts[0];
            bindGroupDesc.entryCount = bindings.size();
            bindGroupDesc.entries = bindings.data();
            bindGroupDesc.label = wgpu::StringView("GBuffer Binding Group");
            auto bg1 = device.createBindGroup(bindGroupDesc);

            if (bg1 == nullptr) throw std::runtime_error("Could not create WebGPU bind group");

            bindGroups.groups["GBuffer"] = bg1;
        },
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
        [](ES::Engine::Core &core) {
            // update cameraBuffer
            wgpu::Queue queue = core.GetResource<wgpu::Queue>();
            auto &camData = core.GetResource<CameraData>();

            auto viewMatrix = glm::lookAt(
                camData.position,
                camData.position + glm::vec3(
                    glm::cos(camData.yaw) * glm::cos(camData.pitch),
                    glm::sin(camData.pitch),
                    glm::sin(camData.yaw) * glm::cos(camData.pitch)
                ),
                camData.up
            );

            auto projectionMatrix = glm::perspective(
                camData.fovY,
                camData.aspectRatio,
                camData.nearPlane,
                camData.farPlane
            );
            Camera cameraBuf;
            cameraBuf.viewProjectionMatrix = projectionMatrix * viewMatrix;
            cameraBuf.invViewProjectionMatrix = glm::inverse(cameraBuf.viewProjectionMatrix);
            cameraBuf.position = camData.position;
            queue.writeBuffer(cameraBuffer, 0, &cameraBuf, sizeof(cameraBuf));
        },
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
