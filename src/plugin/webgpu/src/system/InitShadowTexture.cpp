#include "InitGBufferTextures.hpp"
#include "structs.hpp"
#include "resource/Window/Window.hpp"
#include "plugin/PluginWindow.hpp"
#include <GLFW/glfw3.h>

namespace ES::Plugin::WebGPU::System {
void InitShadowTexture(ES::Engine::Core &core) {
    wgpu::Device device = core.GetResource<wgpu::Device>();
    auto &textureManager = core.GetResource<TextureManager>();
    const std::string name = "shadows";

    auto &textureShadows = textureManager.Add(entt::hashed_string(name.c_str()));

    wgpu::TextureDescriptor textureDesc(wgpu::Default);
    const std::string textureName = fmt::format("Texture::{}", name);
    textureDesc.label = wgpu::StringView(textureName.c_str());
    textureDesc.size = { 2048, 2048, 1 /* 1 for now but we would have to set this to the number of directional lights */ }; // 2048 is a pretty big resolution, if we want a better shadow quality we need to implement a cascade shadows map (little high quality (close shadows), middle average quality (medium distances shadows), big low quality (far shadows))
    textureDesc.format = wgpu::TextureFormat::Depth32Float;
    textureDesc.usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::CopySrc; // CopySrc for debug

    wgpu::TextureViewDescriptor textureViewDesc;
    const std::string textureViewName = fmt::format("TextureView::{}", name);
    textureViewDesc.label = wgpu::StringView(textureViewName.c_str());
    textureViewDesc.format = textureDesc.format;
    textureViewDesc.dimension = wgpu::TextureViewDimension::_2D;
    textureViewDesc.aspect = wgpu::TextureAspect::DepthOnly;
    textureViewDesc.baseMipLevel = 0;
    textureViewDesc.mipLevelCount = 1;
    textureViewDesc.baseArrayLayer = 0;
    textureViewDesc.arrayLayerCount = 1;
    textureViewDesc.usage = textureDesc.usage;

    textureShadows.texture = device.createTexture(textureDesc);
    textureShadows.textureView = textureShadows.texture.createView(textureViewDesc);


}
}
