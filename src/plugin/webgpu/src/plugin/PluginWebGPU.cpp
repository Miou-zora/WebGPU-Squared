#include "plugin/PluginWindow.hpp"
#include "RenderingPipeline.hpp"
#include "scheduler/Shutdown.hpp"
#include "WebGPU.hpp"
#include <lodepng.h>
#include <chrono>

size_t lightIndex = 0;

void DumpDepthTextureAsPNG(
    wgpu::Device& device,
    wgpu::Queue&  queue,
    wgpu::Texture& depthTexture,
    uint32_t      width,
    uint32_t      height,
    const char*   filename,
    uint32_t      arrayLayer = 0  // Ajouter paramètre pour spécifier la couche
) {
    // 1. Create a buffer for readback (must be 256-byte row aligned)
    uint32_t bytesPerPixel = 4;  // we’ll store RGBA8
    uint32_t unalignedBytesPerRow = width * bytesPerPixel;
    uint32_t alignedBytesPerRow = ((unalignedBytesPerRow + 255) / 256) * 256;
    uint64_t bufferSize = uint64_t(alignedBytesPerRow) * height;

    wgpu::BufferDescriptor bufDesc(wgpu::Default);
    bufDesc.size = bufferSize;
    bufDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead;
    bufDesc.label = wgpu::StringView("Depth Readback Buffer");
    wgpu::Buffer readbackBuffer = device.createBuffer(bufDesc);

    // 2. Copy texture → buffer (API v24: TexelCopyTextureInfo / TexelCopyBufferInfo)
    wgpu::CommandEncoder encoder = device.createCommandEncoder();

    wgpu::TexelCopyTextureInfo srcView(wgpu::Default);
    srcView.texture = depthTexture;
    srcView.mipLevel = 0;
    srcView.origin = {0, 0, arrayLayer};  // Spécifier la couche array à copier
    srcView.aspect = wgpu::TextureAspect::DepthOnly;

    wgpu::TexelCopyBufferInfo dstView(wgpu::Default);
    dstView.buffer = readbackBuffer;
    dstView.layout.offset = 0;
    dstView.layout.bytesPerRow = alignedBytesPerRow;
    dstView.layout.rowsPerImage = height; // tightly packed (full texture)

    wgpu::Extent3D copySize(width, height, 1);

    encoder.copyTextureToBuffer(srcView, dstView, copySize);
    auto cmd = encoder.finish();
    queue.submit(1, &cmd);
    cmd.release();

    // 3. Map buffer and read floats
    // 3. Map buffer and read floats (async callback)
    struct CallbackData { wgpu::Buffer buffer; uint32_t width; uint32_t height; uint32_t alignedBytesPerRow; const char* filename; };
    auto *cbData = new CallbackData{ readbackBuffer, width, height, alignedBytesPerRow, filename };
    wgpu::BufferMapCallbackInfo cbInfo(wgpu::Default);
    cbInfo.mode = wgpu::CallbackMode::AllowSpontaneous;
    auto mapCallback = +[](WGPUMapAsyncStatus status, WGPUStringView /*message*/, void* userdata1, void* userdata2) {
        CallbackData* data = reinterpret_cast<CallbackData*>(userdata1);
        if (status != WGPUMapAsyncStatus_Success) { delete data; return; }
        auto &buf = data->buffer;
        uint8_t* mapped = static_cast<uint8_t*>(buf.getMappedRange(0, buf.getSize()));
        std::vector<unsigned char> pngData(data->width * data->height * 4);
        for (uint32_t y = 0; y < data->height; ++y) {
            float const* rowFloats = reinterpret_cast<float const*>(mapped + y * data->alignedBytesPerRow);
            for (uint32_t x = 0; x < data->width; ++x) {
                float depth = rowFloats[x];
                unsigned char v = static_cast<unsigned char>(std::min(std::max(depth, 0.0f), 1.0f) * 255.0f);
                size_t idx = 4 * (y * data->width + x);
                pngData[idx + 0] = v;
                pngData[idx + 1] = v;
                pngData[idx + 2] = v;
                pngData[idx + 3] = 255;
            }
        }
        buf.unmap();
        unsigned error = lodepng::encode(data->filename, pngData, data->width, data->height);
        if (error) {
            fprintf(stderr, "PNG encode error %u: %s\n", error, lodepng_error_text(error));
        }
        delete data;
    };
    cbInfo.callback = mapCallback;
    cbInfo.userdata1 = cbData;
    readbackBuffer.mapAsync(wgpu::MapMode::Read, 0, bufferSize, cbInfo);
}


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
        System::InitializeShadowPipeline,
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
        System::InitShadowTexture,
        System::CreateBindingGroupGBuffer,
        System::CreateBindingGroupDeferred,
        [](ES::Engine::Core &core) {
            core.GetResource<RenderGraph>().AddMultipleRenderPass(
                MultipleRenderPassData{
                    .name = "MultipleShadowPass",
                    .pass = RenderPassData{
                        .name = "ShadowPass",
                        .shaderName = "ShadowPass",
                        .pipelineType = PipelineType::_3D,
                        .loadOp = wgpu::LoadOp::Clear,
                        .clearColor = [](ES::Engine::Core &core) -> glm::vec4 {
                            return glm::vec4(0, 0, 0, 0);
                        },
                        .outputColorTextureName = {},
                        .outputDepthTextureName = "shadows",
                        .bindGroups = {
                            { .groupIndex = 0, .type = BindGroupsLinks::AssetType::BindGroup, .name = "GBufferUniforms" },
                        },
                        .perEntityCallback = [](wgpu::RenderPassEncoder &renderPass, ES::Engine::Core &core, ES::Plugin::WebGPU::Component::Mesh &mesh, ES::Plugin::Object::Component::Transform &transform, ES::Engine::Entity entity) {
                            auto &queue = core.GetResource<wgpu::Queue>();
                            auto &textures = core.GetResource<TextureManager>();
                            entt::hashed_string textureName = entt::hashed_string("DEFAULT_TEXTURE");
                            if (mesh.textures.size() > 0 && textures.Contains(mesh.textures[0])) {
                                textureName = mesh.textures[0];
                            }
                            auto texture = textures.Get(textureName);
                            queue.writeBuffer(mesh.transformIndexBuffer, 0, &entityIndex, sizeof(uint32_t));
                            renderPass.setBindGroup(1, additionalDirectionalLights[lightIndex].bindGroup, 0, nullptr);
                            renderPass.setVertexBuffer(1, mesh.transformIndexBuffer, 0, sizeof(uint32_t));
                            entityIndex++;
                        }
                    },
                    .getNumberOfPass = [](ES::Engine::Core &core) -> size_t {
                        size_t count = 0;
                        auto &lights = core.GetResource<std::vector<Light>>();
                        for (const auto &light : lights) if (light.type == Light::Type::Directional) count++;

                        return count;
                    },
                    .preMultiplePassCallback = [](ES::Engine::Core &, RenderPassData &) {
                        lightIndex = 0;
                        entityIndex = 0;
                    },
                    .prePassCallback = [](ES::Engine::Core &core, RenderPassData &) {
                        auto &textureManager = core.GetResource<TextureManager>();
                        auto &textureShadows = textureManager.Get(entt::hashed_string("shadows"));
                        const std::string name = "shadows";
                        wgpu::TextureViewDescriptor textureViewDesc;
                        const std::string textureViewName = fmt::format("TextureView::{}", name);

                        if (textureShadows.textureView) textureShadows.textureView.release();

                        textureViewDesc.label = wgpu::StringView(textureViewName.c_str());
                        textureViewDesc.format = wgpu::TextureFormat::Depth32Float;
                        textureViewDesc.dimension = wgpu::TextureViewDimension::_2D;
                        textureViewDesc.aspect = wgpu::TextureAspect::DepthOnly;
                        textureViewDesc.baseMipLevel = 0;
                        textureViewDesc.mipLevelCount = 1;
                        textureViewDesc.baseArrayLayer = lightIndex;
                        textureViewDesc.arrayLayerCount = 1;
                        textureViewDesc.usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::RenderAttachment;

                        textureShadows.textureView = textureShadows.texture.createView(textureViewDesc);
                    },
                    .postPassCallback = [](ES::Engine::Core &, RenderPassData &) {
                        lightIndex++;
                    },
                    .postMultiplePassCallback = [](ES::Engine::Core &core, RenderPassData &) {
                        auto &textureManager = core.GetResource<TextureManager>();
                        auto &textureShadows = textureManager.Get(entt::hashed_string("shadows"));
                        const std::string name = "shadows";
                        wgpu::TextureViewDescriptor textureViewDesc;
                        const std::string textureViewName = fmt::format("TextureView::{}", name);
                        auto &device = core.GetResource<wgpu::Device>();

                        if (textureShadows.textureView) textureShadows.textureView.release();

                        textureViewDesc.label = wgpu::StringView(textureViewName.c_str());
                        textureViewDesc.format = wgpu::TextureFormat::Depth32Float;
                        textureViewDesc.dimension = wgpu::TextureViewDimension::_2DArray;
                        textureViewDesc.aspect = wgpu::TextureAspect::DepthOnly;
                        textureViewDesc.baseMipLevel = 0;
                        textureViewDesc.mipLevelCount = 1;
                        textureViewDesc.baseArrayLayer = 0;
                        textureViewDesc.arrayLayerCount = std::max(1lu, lightIndex);
                        textureViewDesc.usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::RenderAttachment;

                        textureShadows.textureView = textureShadows.texture.createView(textureViewDesc);

                        if (textureShadows.bindGroup) textureShadows.bindGroup.release();

                        wgpu::BindGroupEntry textureBinding(wgpu::Default);
                        textureBinding.binding = 0;
                        textureBinding.textureView = textureShadows.textureView;

                        if (additionalDirectionalLightsSampler == nullptr) {
                            wgpu::SamplerDescriptor samplerDesc(wgpu::Default);
                            samplerDesc.maxAnisotropy = 1;
                            samplerDesc.compare = wgpu::CompareFunction::Less;
                            additionalDirectionalLightsSampler = device.createSampler(samplerDesc);
                        }

                        wgpu::BindGroupEntry samplerBinding(wgpu::Default);
		                samplerBinding.binding = 1;
                        samplerBinding.sampler = additionalDirectionalLightsSampler;

                        std::array<wgpu::BindGroupEntry, 2> bindings = { textureBinding, samplerBinding };

                        wgpu::BindGroupDescriptor bindGroupDesc(wgpu::Default);
                        bindGroupDesc.layout = core.GetResource<Pipelines>().renderPipelines["Deferred"].bindGroupLayouts[3];
                        bindGroupDesc.entryCount = bindings.size();
                        bindGroupDesc.entries = bindings.data();
                        bindGroupDesc.label = wgpu::StringView("Shadows Bind Group");
                        textureShadows.bindGroup = device.createBindGroup(bindGroupDesc);

                        entityIndex = 0;
                        lightIndex = 0;

                        // Uncomment this to create a file to debug shadowmaps
                        // static auto lastDumpTime = std::chrono::steady_clock::now();
                        // auto now = std::chrono::steady_clock::now();
                        // if (now - lastDumpTime >= std::chrono::seconds(1)) {
                        //     auto &queue = core.GetResource<wgpu::Queue>();
                        //     // Dumper la première couche de la texture array (index 0)
                        //     DumpDepthTextureAsPNG(device, queue, textureShadows.texture, 2048, 2048, "depth.png", 0);
                        //     lastDumpTime = now;
                        // }
                    }
                }
            );
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
                        { .groupIndex = 2, .type = BindGroupsLinks::AssetType::BindGroup, .name = "GBufferUniforms" },
                    },
                    .perEntityCallback = [](wgpu::RenderPassEncoder &renderPass, ES::Engine::Core &core, ES::Plugin::WebGPU::Component::Mesh &mesh, ES::Plugin::Object::Component::Transform &transform, ES::Engine::Entity entity) {
                        auto &queue = core.GetResource<wgpu::Queue>();
                        auto &textures = core.GetResource<TextureManager>();
                        entt::hashed_string textureName = entt::hashed_string("DEFAULT_TEXTURE");
                        if (mesh.textures.size() > 0 && textures.Contains(mesh.textures[0])) {
                            textureName = mesh.textures[0];
                        }
                        auto texture = textures.Get(textureName);
                        queue.writeBuffer(mesh.transformIndexBuffer, 0, &entityIndex, sizeof(uint32_t));
                        renderPass.setVertexBuffer(1, mesh.transformIndexBuffer, 0, sizeof(uint32_t));
                        renderPass.setBindGroup(1, texture.bindGroup, 0, nullptr);
                        entityIndex++;
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
                        { .groupIndex = 2, .type = BindGroupsLinks::AssetType::BindGroup, .name = "DeferredGroup2" },
                        { .groupIndex = 3, .type = BindGroupsLinks::AssetType::TextureView, .name = "shadows" }
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
        [](ES::Engine::Core &core) {
            entityIndex = 0;
        },
        System::UpdateBufferUniforms,
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

