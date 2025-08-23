#include "InitGBufferTextures.hpp"
#include "structs.hpp"
#include "Window.hpp"
#include "PluginWindow.hpp"
#include <GLFW/glfw3.h>

namespace ES::Plugin::WebGPU::System {
void InitGBufferTextures(ES::Engine::Core &core) {
    wgpu::Device device = core.GetResource<wgpu::Device>();
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

    core.GetResource<WindowResizeCallbacks>().callbacks.push_back([](ES::Engine::Core &core, int width, int height) {
        auto &textureManager = core.GetResource<TextureManager>();
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
}
}
