#include "WGPURenderInterface.hpp"

namespace ES::Plugin::Rmlui::WebGPU::System
{
    WGPURenderInterface::WGPURenderInterface(ES::Engine::Core &core) : _core(core) {}

    Rml::CompiledGeometryHandle WGPURenderInterface::CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices)
    {
        return 0;
    };
    void WGPURenderInterface::RenderGeometry(Rml::CompiledGeometryHandle handle, Rml::Vector2f translation, Rml::TextureHandle texture_handle) {};
    void WGPURenderInterface::ReleaseGeometry(Rml::CompiledGeometryHandle handle) {};
    Rml::TextureHandle WGPURenderInterface::LoadTexture(Rml::Vector2i &texture_dimensions, const Rml::String &source) {
        return 0;
    };
    Rml::TextureHandle WGPURenderInterface::GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i dimensions) {
        return 0;
    };
    Rml::TextureHandle WGPURenderInterface::CreateTexture(Rml::Span<const Rml::byte> source_data, Rml::Vector2i source_dimensions) {
        return 0;
    };
    void WGPURenderInterface::ReleaseTexture(Rml::TextureHandle handle) {};
    void WGPURenderInterface::EnableScissorRegion(bool enable) {};
    void WGPURenderInterface::SetScissorRegion(Rml::Rectanglei region) {};
    void WGPURenderInterface::SetScissor(Rml::Rectanglei region, bool vertically_flip) {};
    void WGPURenderInterface::BeginFrame() {};
    void WGPURenderInterface::EndFrame() {};
    void WGPURenderInterface::DrawFullscreenQuad() {};
    void WGPURenderInterface::SetTransform(const Rml::Matrix4f *new_transform) {};
}
