#pragma once

#include <RmlUi/Core.h>
#include "core/Core.hpp"
#include "utils/IRenderer.hpp"
#include <webgpu.hpp>
#include <vector>
#include <unordered_map>
#include <memory>

namespace ES::Plugin::Rmlui::WebGPU::System
{
    struct RmlUIVertex {
        float position[2];  // x, y
        float color[4];     // r, g, b, a
        float texCoord[2];  // u, v
    };

    struct CompiledGeometry {
        wgpu::Buffer vertexBuffer = nullptr;
        wgpu::Buffer indexBuffer = nullptr;
        uint32_t indexCount = 0;
        uint32_t vertexCount = 0;
    };

    struct RmlUITexture {
        wgpu::Texture texture = nullptr;
        wgpu::TextureView textureView = nullptr;
        wgpu::Sampler sampler = nullptr;
        wgpu::BindGroup bindGroup = nullptr;
        Rml::Vector2i dimensions;
    };

    struct QueuedGeometry {
        Rml::CompiledGeometryHandle handle;
        Rml::Vector2f translation;
        Rml::TextureHandle textureHandle;
    };

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
        
        void SetRenderPassEncoder(wgpu::RenderPassEncoder renderPass);
        void RenderQueuedGeometry();
        
        // Debug methods
        size_t GetRenderQueueSize() const { return _renderQueue.size(); }

    private:
        ES::Engine::Core &_core;
        
        std::unordered_map<Rml::CompiledGeometryHandle, std::unique_ptr<CompiledGeometry>> _compiledGeometries;
        Rml::CompiledGeometryHandle _nextGeometryHandle = 1;
        
        std::unordered_map<Rml::TextureHandle, std::unique_ptr<RmlUITexture>> _textures;
        Rml::TextureHandle _nextTextureHandle = 1;
        
        bool _scissorEnabled = false;
        Rml::Rectanglei _scissorRegion;
        Rml::Matrix4f _transform = Rml::Matrix4f::Identity();
        
        wgpu::RenderPipeline _uiPipeline = nullptr;
        wgpu::BindGroupLayout _textureBindGroupLayout = nullptr;
        wgpu::BindGroupLayout _uniformBindGroupLayout = nullptr;
        wgpu::Buffer _uniformBuffer = nullptr;
        wgpu::BindGroup _uniformBindGroup = nullptr;
        
        // Default white texture for elements without textures
        wgpu::Texture _defaultTexture = nullptr;
        wgpu::TextureView _defaultTextureView = nullptr;
        wgpu::Sampler _defaultSampler = nullptr;
        wgpu::BindGroup _defaultTextureBindGroup = nullptr;
        
        std::vector<QueuedGeometry> _renderQueue;
        wgpu::RenderPassEncoder _currentRenderPass = nullptr;
        
        void InitializePipeline();
        void CreateUniformBuffer();
        void CreateDefaultTexture();
        RmlUIVertex ConvertVertex(const Rml::Vertex& rmlVertex);
        wgpu::Buffer CreateVertexBuffer(const std::vector<RmlUIVertex>& vertices);
        wgpu::Buffer CreateIndexBuffer(const std::vector<uint32_t>& indices);
        RmlUITexture* GetTexture(Rml::TextureHandle handle);
    };
}
