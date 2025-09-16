#pragma once

#include <RmlUi/Core.h>
#include "core/Core.hpp"
#include "utils/IRenderer.hpp"

namespace ES::Plugin::Rmlui::WebGPU::System
{
    class WGPURenderInterface : public ES::Plugin::Rmlui::IRenderer
    {
    public:
        WGPURenderInterface() = delete;
        explicit WGPURenderInterface(ES::Engine::Core &core);
        ~WGPURenderInterface() = default;

        Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) override;
        void RenderGeometry(Rml::CompiledGeometryHandle handle, Rml::Vector2f translation, Rml::TextureHandle texture_handle) override;
        void ReleaseGeometry(Rml::CompiledGeometryHandle handle) override;
        Rml::TextureHandle LoadTexture(Rml::Vector2i &texture_dimensions, const Rml::String &source) override;
        Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i dimensions) override;
        Rml::TextureHandle CreateTexture(Rml::Span<const Rml::byte> source_data, Rml::Vector2i source_dimensions);
        void ReleaseTexture(Rml::TextureHandle handle) override;
        void EnableScissorRegion(bool enable) override;
        void SetScissorRegion(Rml::Rectanglei region) override;
        void SetScissor(Rml::Rectanglei region, bool vertically_flip);
        void BeginFrame() override;
        void EndFrame() override;
        void DrawFullscreenQuad();
        void SetTransform(const Rml::Matrix4f *new_transform) override;
    private:
        ES::Engine::Core &_core;
    };
}
