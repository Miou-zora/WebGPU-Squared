#include "InitGBufferTextures.hpp"
#include "structs.hpp"
#include "resource/window/Window.hpp"
#include "plugin/PluginWindow.hpp"
#include <GLFW/glfw3.h>

namespace ES::Plugin::WebGPU::System {

static void CreateSkyboxBuffers(ES::Engine::Core &core)
{
    wgpu::Device device = core.GetResource<wgpu::Device>();
    auto &textureManager = core.GetResource<TextureManager>();
    auto &window = core.GetResource<ES::Plugin::Window::Resource::Window>();
    const std::array<std::string, 6> skyboxFaces = {
        "assets/skybox/right.jpg",
        "assets/skybox/left.jpg",
        "assets/skybox/top.jpg",
        "assets/skybox/bottom.jpg",
        "assets/skybox/front.jpg",
        "assets/skybox/back.jpg"
    };

    struct ImageCube {
        unsigned char* data[6];
        int width;
        int height;
        int channels;
    } imageCube;

    stbi_set_flip_vertically_on_load(false);
    for (int i = 0; i < 6; ++i) {
        std::string path = skyboxFaces[i];
        unsigned char *pixelData = stbi_load(path.c_str(), &imageCube.width, &imageCube.height, &imageCube.channels, 4 /* force 4 channels */);
        if (!pixelData) {
            throw std::runtime_error("Failed to load skybox texture");
        }
        imageCube.data[i] = pixelData;
    }
    stbi_set_flip_vertically_on_load(true);

    auto &skyboxTexture = textureManager.Add("SkyboxTexture");

    wgpu::TextureDescriptor textureDesc(wgpu::Default);
    textureDesc.label = wgpu::StringView("SkyboxTexture");
    textureDesc.size = { static_cast<uint32_t>(imageCube.width), static_cast<uint32_t>(imageCube.height), 6 };
    textureDesc.format = wgpu::TextureFormat::RGBA8Unorm;
    textureDesc.usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::CopyDst;
    textureDesc.dimension = wgpu::TextureDimension::_2D;
    skyboxTexture.texture = device.createTexture(textureDesc);

    for (uint32_t i = 0; i < 6; ++i) {
        const unsigned char* data = imageCube.data[i];
        wgpu::TexelCopyTextureInfo srcView(wgpu::Default);
        srcView.texture = skyboxTexture.texture;
        srcView.mipLevel = 0;
        srcView.origin = { 0, 0, i };

        wgpu::TexelCopyBufferLayout layout(wgpu::Default);
        layout.bytesPerRow = 4 * imageCube.width;
        layout.rowsPerImage = imageCube.height;
        layout.offset = 0;

        wgpu::Extent3D copySize(imageCube.width, imageCube.height, 1);

        device.getQueue().writeTexture(srcView, data, imageCube.width * imageCube.height * 4,layout, copySize);
        stbi_image_free(imageCube.data[i]);
    }

    wgpu::TextureViewDescriptor textureViewDesc(wgpu::Default);
    textureViewDesc.dimension = wgpu::TextureViewDimension::Cube;
    textureViewDesc.format = textureDesc.format;
    textureViewDesc.baseMipLevel = 0;
    textureViewDesc.mipLevelCount = 1;
    textureViewDesc.baseArrayLayer = 0;
    textureViewDesc.arrayLayerCount = 6;
    textureViewDesc.aspect = wgpu::TextureAspect::All;
    skyboxTexture.textureView = skyboxTexture.texture.createView(textureViewDesc);

    wgpu::SamplerDescriptor samplerDesc(wgpu::Default);
    samplerDesc.label = wgpu::StringView("Skybox Sampler");
    samplerDesc.maxAnisotropy = 1;
    samplerDesc.magFilter = wgpu::FilterMode::Linear;
    samplerDesc.minFilter = wgpu::FilterMode::Linear;
    skyboxTexture.sampler = device.createSampler(samplerDesc);
}


static void InitSkyboxOutputTexture(ES::Engine::Core &core)
{
    auto &device = core.GetResource<wgpu::Device>();
    auto &pipelineData = core.GetResource<Pipelines>().renderPipelines["Deferred"];
    auto &textureManager = core.GetResource<TextureManager>();
    auto &window = core.GetResource<ES::Plugin::Window::Resource::Window>();
    auto &bindGroups = core.GetResource<BindGroups>();

    int frameBufferSizeX, frameBufferSizeY;
    glfwGetFramebufferSize(window.GetGLFWWindow(), &frameBufferSizeX, &frameBufferSizeY);

    auto &texture = textureManager.Add("SkyboxOutput");

    wgpu::TextureDescriptor textureDesc(wgpu::Default);
    textureDesc.label = wgpu::StringView("SkyboxOutput");
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
	bindGroupDesc.layout = pipelineData.bindGroupLayouts[4];
	bindGroupDesc.entryCount = bindings.size();
	bindGroupDesc.entries = bindings.data();
	bindGroupDesc.label = wgpu::StringView("Deferred Binding Group Textures");
	auto bg = device.createBindGroup(bindGroupDesc);

	if (bg == nullptr) throw std::runtime_error("Could not create WebGPU bind group");

	bindGroups.groups["DeferredGroup4"] = bg;
}

static void CreateSkyboxCubeBuffer(ES::Engine::Core &core)
{
    wgpu::Device device = core.GetResource<wgpu::Device>();
    auto &queue = core.GetResource<wgpu::Queue>();

    wgpu::BufferDescriptor bufferDesc(wgpu::Default);
    bufferDesc.size = skyboxCube.size() * sizeof(float);
    bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
    bufferDesc.label = wgpu::StringView("Skybox Cube Transform Buffer");
    skyboxCubeBuffer = device.createBuffer(bufferDesc);

    queue.writeBuffer(skyboxCubeBuffer, 0, skyboxCube.data(), bufferDesc.size);
}

static void CreateSkyboxUniformBuffer(ES::Engine::Core &core)
{
    wgpu::Device device = core.GetResource<wgpu::Device>();
    auto &queue = core.GetResource<wgpu::Queue>();

    wgpu::BufferDescriptor bufferDesc(wgpu::Default);
    bufferDesc.size = sizeof(glm::mat4);
    bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
    bufferDesc.label = wgpu::StringView("Skybox Transform Buffer");
    skyboxBuffer = device.createBuffer(bufferDesc);

    glm::mat4 identity = glm::mat4(1.0f);
    queue.writeBuffer(skyboxBuffer, 0, &identity, sizeof(identity));
}

void InitSkyboxBuffers(ES::Engine::Core &core) {
    CreateSkyboxUniformBuffer(core);
    CreateSkyboxBuffers(core);
    CreateSkyboxCubeBuffer(core);

    // core.GetResource<WindowResizeCallbacks>().callbacks.push_back([](ES::Engine::Core &core, int width, int height) {
    //     auto &textureManager = core.GetResource<TextureManager>();
    //     auto &device = core.GetResource<wgpu::Device>();

    //     {
    //         auto &skyboxTexture = textureManager.Get("SkyboxTexture");
    //         skyboxTexture.textureView.release();
    //         skyboxTexture.texture.destroy();
    //         skyboxTexture.texture.release();
    //         skyboxTexture.sampler.release();
    //     }

    //     textureManager.Remove("SkyboxTexture");

    //     CreateSkyboxBuffers(core);
    // });

    InitSkyboxOutputTexture(core);

    core.GetResource<WindowResizeCallbacks>().callbacks.push_back([](ES::Engine::Core &core, int width, int height) {
        auto &textureManager = core.GetResource<TextureManager>();
        auto &device = core.GetResource<wgpu::Device>();
        auto &bindGroups = core.GetResource<BindGroups>();

		{
			auto &textureNormal = textureManager.Get("SkyboxOutput");
			textureNormal.textureView.release();
			textureNormal.texture.destroy();
			textureNormal.texture.release();

            bindGroups.groups["DeferredGroup4"].release();
		}

        textureManager.Remove("SkyboxOutput");

		InitSkyboxOutputTexture(core);
    });
}
}
