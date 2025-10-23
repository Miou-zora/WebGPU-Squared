#include "WGPURenderInterface.hpp"
#include <stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include "../../webgpu/src/util/structs.hpp"
#include "../../webgpu/src/WebGPU.hpp"

namespace ES::Plugin::Rmlui::WebGPU::System
{
    WGPURenderInterface::WGPURenderInterface(ES::Engine::Core &core) : _core(core) 
    {
        InitializePipeline();
        CreateUniformBuffer();
        CreateDefaultTexture();
    }

    void WGPURenderInterface::InitializePipeline()
    {
        auto device = _core.GetResource<wgpu::Device>();
        auto surface = _core.GetResource<wgpu::Surface>();
        auto surfaceFormat = _core.GetResource<wgpu::SurfaceCapabilities>().formats[0];
        
        wgpu::BindGroupLayoutEntry uniformBinding{};
        uniformBinding.binding = 0;
        uniformBinding.visibility = wgpu::ShaderStage::Vertex;
        uniformBinding.buffer.type = wgpu::BufferBindingType::Uniform;
        uniformBinding.buffer.hasDynamicOffset = false;
        uniformBinding.buffer.minBindingSize = sizeof(glm::mat4);
        uniformBinding.texture = wgpu::TextureBindingLayout{};
        uniformBinding.sampler = wgpu::SamplerBindingLayout{};
        uniformBinding.storageTexture = wgpu::StorageTextureBindingLayout{};

        std::array<wgpu::BindGroupLayoutEntry, 1> uniformBindGroupLayoutEntries = { uniformBinding };
        
        wgpu::BindGroupLayoutDescriptor uniformBindGroupLayoutDesc(wgpu::Default);
        uniformBindGroupLayoutDesc.entryCount = uniformBindGroupLayoutEntries.size();
        uniformBindGroupLayoutDesc.entries = uniformBindGroupLayoutEntries.data();
        uniformBindGroupLayoutDesc.label = wgpu::StringView("RmlUI Uniform Bind Group Layout");
        
        _uniformBindGroupLayout = device.createBindGroupLayout(uniformBindGroupLayoutDesc);

        wgpu::BindGroupLayoutEntry textureBinding{};
        textureBinding.binding = 0;
        textureBinding.visibility = wgpu::ShaderStage::Fragment;
        textureBinding.texture.sampleType = wgpu::TextureSampleType::Float;
        textureBinding.texture.viewDimension = wgpu::TextureViewDimension::_2D;
        textureBinding.buffer = wgpu::BufferBindingLayout{};
        textureBinding.sampler = wgpu::SamplerBindingLayout{};
        textureBinding.storageTexture = wgpu::StorageTextureBindingLayout{};

        wgpu::BindGroupLayoutEntry samplerBinding{};
        samplerBinding.binding = 1;
        samplerBinding.visibility = wgpu::ShaderStage::Fragment;
        samplerBinding.sampler.type = wgpu::SamplerBindingType::Filtering;
        samplerBinding.buffer = wgpu::BufferBindingLayout{};
        samplerBinding.texture = wgpu::TextureBindingLayout{};
        samplerBinding.storageTexture = wgpu::StorageTextureBindingLayout{};

        std::array<wgpu::BindGroupLayoutEntry, 2> textureBindGroupLayoutEntries = { textureBinding, samplerBinding };
        
        wgpu::BindGroupLayoutDescriptor textureBindGroupLayoutDesc(wgpu::Default);
        textureBindGroupLayoutDesc.entryCount = textureBindGroupLayoutEntries.size();
        textureBindGroupLayoutDesc.entries = textureBindGroupLayoutEntries.data();
        textureBindGroupLayoutDesc.label = wgpu::StringView("RmlUI Texture Bind Group Layout");
        
        _textureBindGroupLayout = device.createBindGroupLayout(textureBindGroupLayoutDesc);

        std::array<WGPUBindGroupLayout, 2> bindGroupLayouts = { _uniformBindGroupLayout, _textureBindGroupLayout };
        
        wgpu::PipelineLayoutDescriptor pipelineLayoutDesc(wgpu::Default);
        pipelineLayoutDesc.bindGroupLayoutCount = bindGroupLayouts.size();
        pipelineLayoutDesc.bindGroupLayouts = bindGroupLayouts.data();
        pipelineLayoutDesc.label = wgpu::StringView("RmlUI Pipeline Layout");
        
        auto pipelineLayout = device.createPipelineLayout(pipelineLayoutDesc);

        const char* shaderSource = R"(
            struct VertexInput {
                @location(0) position: vec2<f32>,
                @location(1) color: vec4<f32>,
                @location(2) texCoord: vec2<f32>,
            }

            struct VertexOutput {
                @builtin(position) position: vec4<f32>,
                @location(0) color: vec4<f32>,
                @location(1) texCoord: vec2<f32>,
            }

            struct Uniforms {
                transform: mat4x4<f32>,
            }

            @group(0) @binding(0) var<uniform> uniforms: Uniforms;
            @group(1) @binding(0) var texture: texture_2d<f32>;
            @group(1) @binding(1) var textureSampler: sampler;

            @vertex
            fn vs_main(input: VertexInput) -> VertexOutput {
                var output: VertexOutput;
                let worldPos = vec4<f32>(input.position, 0.0, 1.0);
                output.position = uniforms.transform * worldPos;
                output.color = input.color;
                output.texCoord = input.texCoord;
                return output;
            }

            @fragment
            fn fs_main(input: VertexOutput) -> @location(0) vec4<f32> {
                let textureColor = textureSample(texture, textureSampler, input.texCoord);
                return input.color * textureColor;
            }
        )";

        wgpu::ShaderSourceWGSL wgslDesc(wgpu::Default);
        wgslDesc.code = wgpu::StringView(shaderSource);
        wgpu::ShaderModuleDescriptor shaderDesc(wgpu::Default);
        shaderDesc.nextInChain = &wgslDesc.chain;
        shaderDesc.label = wgpu::StringView("RmlUI Shader");
        
        auto shaderModule = device.createShaderModule(shaderDesc);

        std::array<wgpu::VertexAttribute, 3> vertexAttributes;
        
        vertexAttributes[0].shaderLocation = 0;
        vertexAttributes[0].format = wgpu::VertexFormat::Float32x2;
        vertexAttributes[0].offset = 0;

        vertexAttributes[1].shaderLocation = 1;
        vertexAttributes[1].format = wgpu::VertexFormat::Float32x4;
        vertexAttributes[1].offset = 2 * sizeof(float);

        vertexAttributes[2].shaderLocation = 2;
        vertexAttributes[2].format = wgpu::VertexFormat::Float32x2;
        vertexAttributes[2].offset = 6 * sizeof(float);

        wgpu::VertexBufferLayout vertexBufferLayout(wgpu::Default);
        vertexBufferLayout.attributeCount = vertexAttributes.size();
        vertexBufferLayout.attributes = vertexAttributes.data();
        vertexBufferLayout.arrayStride = 8 * sizeof(float);
        vertexBufferLayout.stepMode = wgpu::VertexStepMode::Vertex;

        wgpu::RenderPipelineDescriptor pipelineDesc(wgpu::Default);
        pipelineDesc.label = wgpu::StringView("RmlUI Render Pipeline");
        pipelineDesc.layout = pipelineLayout;

        pipelineDesc.vertex.module = shaderModule;
        pipelineDesc.vertex.entryPoint = wgpu::StringView("vs_main");
        pipelineDesc.vertex.bufferCount = 1;
        pipelineDesc.vertex.buffers = &vertexBufferLayout;

        wgpu::FragmentState fragmentState(wgpu::Default);
        fragmentState.module = shaderModule;
        fragmentState.entryPoint = wgpu::StringView("fs_main");
        
        wgpu::ColorTargetState colorTarget(wgpu::Default);
        colorTarget.format = surfaceFormat;
        colorTarget.writeMask = wgpu::ColorWriteMask::All;
        
        fragmentState.targetCount = 1;
        fragmentState.targets = &colorTarget;
        pipelineDesc.fragment = &fragmentState;

        pipelineDesc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
        pipelineDesc.primitive.stripIndexFormat = wgpu::IndexFormat::Undefined;
        pipelineDesc.primitive.frontFace = wgpu::FrontFace::CCW;
        pipelineDesc.primitive.cullMode = wgpu::CullMode::None;

        _uiPipeline = device.createRenderPipeline(pipelineDesc);
        
        auto &pipelines = _core.GetResource<Pipelines>();
        pipelines.renderPipelines["RmlUI"] = PipelineData{
            .pipeline = _uiPipeline,
            .bindGroupLayouts = {_uniformBindGroupLayout, _textureBindGroupLayout},
            .layout = pipelineLayout
        };
        
        shaderModule.release();
    }

    void WGPURenderInterface::CreateUniformBuffer()
    {
        auto device = _core.GetResource<wgpu::Device>();
        
        wgpu::BufferDescriptor bufferDesc(wgpu::Default);
        bufferDesc.size = sizeof(glm::mat4);
        bufferDesc.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
        bufferDesc.label = wgpu::StringView("RmlUI Uniform Buffer");
        
        _uniformBuffer = device.createBuffer(bufferDesc);
        
        wgpu::BindGroupEntry uniformBinding(wgpu::Default);
        uniformBinding.binding = 0;
        uniformBinding.buffer = _uniformBuffer;
        uniformBinding.offset = 0;
        uniformBinding.size = sizeof(glm::mat4);
        
        std::array<wgpu::BindGroupEntry, 1> uniformBindings = { uniformBinding };
        
        wgpu::BindGroupDescriptor bindGroupDesc(wgpu::Default);
        bindGroupDesc.layout = _uniformBindGroupLayout;
        bindGroupDesc.entryCount = uniformBindings.size();
        bindGroupDesc.entries = uniformBindings.data();
        bindGroupDesc.label = wgpu::StringView("RmlUI Uniform Bind Group");
        
        _uniformBindGroup = device.createBindGroup(bindGroupDesc);
        
        auto &bindGroups = _core.GetResource<BindGroups>();
        bindGroups.groups["RmlUIUniforms"] = _uniformBindGroup;
    }

    void WGPURenderInterface::CreateDefaultTexture()
    {
        auto device = _core.GetResource<wgpu::Device>();
        
        wgpu::TextureDescriptor textureDesc(wgpu::Default);
        textureDesc.size = {1, 1, 1};
        textureDesc.format = wgpu::TextureFormat::RGBA8Unorm;
        textureDesc.usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst;
        textureDesc.label = wgpu::StringView("RmlUI Default Texture");
        
        _defaultTexture = device.createTexture(textureDesc);
        
        wgpu::TextureViewDescriptor viewDesc(wgpu::Default);
        viewDesc.format = wgpu::TextureFormat::RGBA8Unorm;
        viewDesc.dimension = wgpu::TextureViewDimension::_2D;
        viewDesc.baseMipLevel = 0;
        viewDesc.mipLevelCount = 1;
        viewDesc.baseArrayLayer = 0;
        viewDesc.arrayLayerCount = 1;
        viewDesc.label = wgpu::StringView("RmlUI Default Texture View");
        
        _defaultTextureView = _defaultTexture.createView(viewDesc);
        
        wgpu::SamplerDescriptor samplerDesc(wgpu::Default);
        samplerDesc.magFilter = wgpu::FilterMode::Linear;
        samplerDesc.minFilter = wgpu::FilterMode::Linear;
        samplerDesc.mipmapFilter = wgpu::MipmapFilterMode::Linear;
        samplerDesc.addressModeU = wgpu::AddressMode::ClampToEdge;
        samplerDesc.addressModeV = wgpu::AddressMode::ClampToEdge;
        samplerDesc.addressModeW = wgpu::AddressMode::ClampToEdge;
        samplerDesc.maxAnisotropy = 1;
        samplerDesc.label = wgpu::StringView("RmlUI Default Sampler");
        
        _defaultSampler = device.createSampler(samplerDesc);
        
        wgpu::BindGroupEntry textureBinding(wgpu::Default);
        textureBinding.binding = 0;
        textureBinding.textureView = _defaultTextureView;
        
        wgpu::BindGroupEntry samplerBinding(wgpu::Default);
        samplerBinding.binding = 1;
        samplerBinding.sampler = _defaultSampler;
        
        std::array<wgpu::BindGroupEntry, 2> textureBindings = { textureBinding, samplerBinding };
        
        wgpu::BindGroupDescriptor bindGroupDesc(wgpu::Default);
        bindGroupDesc.layout = _textureBindGroupLayout;
        bindGroupDesc.entryCount = textureBindings.size();
        bindGroupDesc.entries = textureBindings.data();
        bindGroupDesc.label = wgpu::StringView("RmlUI Default Texture Bind Group");
        
        _defaultTextureBindGroup = device.createBindGroup(bindGroupDesc);
        
        uint8_t whitePixel[4] = {255, 255, 255, 255};
        auto queue = _core.GetResource<wgpu::Queue>();
        
        wgpu::TexelCopyTextureInfo destination(wgpu::Default);
        destination.texture = _defaultTexture;
        destination.mipLevel = 0;
        destination.origin = {0, 0, 0};
        
        wgpu::TexelCopyBufferLayout sourceLayout(wgpu::Default);
        sourceLayout.offset = 0;
        sourceLayout.bytesPerRow = 4;
        sourceLayout.rowsPerImage = 1;
        
        queue.writeTexture(destination, whitePixel, 4, sourceLayout, {1, 1, 1});
    }

    RmlUIVertex WGPURenderInterface::ConvertVertex(const Rml::Vertex& rmlVertex)
    {
        RmlUIVertex vertex;
        vertex.position[0] = rmlVertex.position.x;
        vertex.position[1] = rmlVertex.position.y;
        vertex.color[0] = rmlVertex.colour.red / 255.0f;
        vertex.color[1] = rmlVertex.colour.green / 255.0f;
        vertex.color[2] = rmlVertex.colour.blue / 255.0f;
        vertex.color[3] = rmlVertex.colour.alpha / 255.0f;
        vertex.texCoord[0] = rmlVertex.tex_coord.x;
        vertex.texCoord[1] = rmlVertex.tex_coord.y;
        return vertex;
    }

    wgpu::Buffer WGPURenderInterface::CreateVertexBuffer(const std::vector<RmlUIVertex>& vertices)
    {
        auto device = _core.GetResource<wgpu::Device>();
        auto queue = _core.GetResource<wgpu::Queue>();
        
        wgpu::BufferDescriptor bufferDesc(wgpu::Default);
        bufferDesc.size = vertices.size() * sizeof(RmlUIVertex);
        bufferDesc.usage = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst;
        bufferDesc.label = wgpu::StringView("RmlUI Vertex Buffer");
        
        auto buffer = device.createBuffer(bufferDesc);
        queue.writeBuffer(buffer, 0, vertices.data(), bufferDesc.size);
        
        return buffer;
    }

    wgpu::Buffer WGPURenderInterface::CreateIndexBuffer(const std::vector<uint32_t>& indices)
    {
        auto device = _core.GetResource<wgpu::Device>();
        auto queue = _core.GetResource<wgpu::Queue>();
        
        wgpu::BufferDescriptor bufferDesc(wgpu::Default);
        bufferDesc.size = indices.size() * sizeof(uint32_t);
        bufferDesc.usage = wgpu::BufferUsage::Index | wgpu::BufferUsage::CopyDst;
        bufferDesc.label = wgpu::StringView("RmlUI Index Buffer");
        
        auto buffer = device.createBuffer(bufferDesc);
        queue.writeBuffer(buffer, 0, indices.data(), bufferDesc.size);
        
        return buffer;
    }

    RmlUITexture* WGPURenderInterface::GetTexture(Rml::TextureHandle handle)
    {
        auto it = _textures.find(handle);
        return (it != _textures.end()) ? it->second.get() : nullptr;
    }

Rml::CompiledGeometryHandle WGPURenderInterface::CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices)
{
    auto handle = _nextGeometryHandle++;
        
        std::vector<RmlUIVertex> rmlVertices;
        rmlVertices.reserve(vertices.size());
        for (const auto& vertex : vertices) {
            rmlVertices.push_back(ConvertVertex(vertex));
        }
        
        std::vector<uint32_t> rmlIndices;
        rmlIndices.reserve(indices.size());
        for (const auto& index : indices) {
            rmlIndices.push_back(static_cast<uint32_t>(index));
        }
        
        auto vertexBuffer = CreateVertexBuffer(rmlVertices);
        auto indexBuffer = CreateIndexBuffer(rmlIndices);
        
        auto geometry = std::make_unique<CompiledGeometry>();
        geometry->vertexBuffer = vertexBuffer;
        geometry->indexBuffer = indexBuffer;
        geometry->indexCount = static_cast<uint32_t>(indices.size());
        geometry->vertexCount = static_cast<uint32_t>(vertices.size());
        
        _compiledGeometries[handle] = std::move(geometry);
        
        return handle;
    }

    void WGPURenderInterface::RenderGeometry(Rml::CompiledGeometryHandle handle, Rml::Vector2f translation, Rml::TextureHandle texture_handle)
    {
        _renderQueue.push_back({handle, translation, texture_handle});
    }

    void WGPURenderInterface::ReleaseGeometry(Rml::CompiledGeometryHandle handle)
    {
        auto it = _compiledGeometries.find(handle);
        if (it != _compiledGeometries.end()) {
            it->second->vertexBuffer.release();
            it->second->indexBuffer.release();
            _compiledGeometries.erase(it);
        }
    }

    Rml::TextureHandle WGPURenderInterface::LoadTexture(Rml::Vector2i &texture_dimensions, const Rml::String &source)
    {
        auto handle = _nextTextureHandle++;
        auto device = _core.GetResource<wgpu::Device>();
        auto queue = _core.GetResource<wgpu::Queue>();
        
        int width, height, channels;
        unsigned char* data = stbi_load(source.c_str(), &width, &height, &channels, 4);
        if (!data) {
            ES::Utils::Log::Error(fmt::format("RmlUI: Failed to load texture: {}", source));
            return 0;
        }
        
        texture_dimensions.x = width;
        texture_dimensions.y = height;
        
        wgpu::TextureDescriptor textureDesc(wgpu::Default);
        textureDesc.label = wgpu::StringView("RmlUI Texture");
        textureDesc.size = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
        textureDesc.format = wgpu::TextureFormat::RGBA8Unorm;
        textureDesc.usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst;
        textureDesc.dimension = wgpu::TextureDimension::_2D;
        
        auto texture = device.createTexture(textureDesc);
        
        wgpu::Extent3D textureSize = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
        wgpu::TexelCopyTextureInfo destination;
        destination.texture = texture;
        destination.mipLevel = 0;
        destination.origin = { 0, 0, 0 };
        destination.aspect = wgpu::TextureAspect::All;
        
        wgpu::TexelCopyBufferLayout sourceLayout;
        sourceLayout.offset = 0;
        sourceLayout.bytesPerRow = 4 * width;
        sourceLayout.rowsPerImage = height;
        
        queue.writeTexture(destination, data, 4 * width * height, sourceLayout, textureSize);
        stbi_image_free(data);
        
        wgpu::TextureViewDescriptor textureViewDesc(wgpu::Default);
        textureViewDesc.format = wgpu::TextureFormat::RGBA8Unorm;
        textureViewDesc.dimension = wgpu::TextureViewDimension::_2D;
        textureViewDesc.baseMipLevel = 0;
        textureViewDesc.mipLevelCount = 1;
        textureViewDesc.baseArrayLayer = 0;
        textureViewDesc.arrayLayerCount = 1;
        
        auto textureView = texture.createView(textureViewDesc);
        
        wgpu::SamplerDescriptor samplerDesc(wgpu::Default);
        samplerDesc.magFilter = wgpu::FilterMode::Linear;
        samplerDesc.minFilter = wgpu::FilterMode::Linear;
        samplerDesc.mipmapFilter = wgpu::MipmapFilterMode::Linear;
        samplerDesc.addressModeU = wgpu::AddressMode::ClampToEdge;
        samplerDesc.addressModeV = wgpu::AddressMode::ClampToEdge;
        samplerDesc.addressModeW = wgpu::AddressMode::ClampToEdge;
        samplerDesc.maxAnisotropy = 1;
        
        auto sampler = device.createSampler(samplerDesc);
        
        wgpu::BindGroupEntry textureBinding(wgpu::Default);
        textureBinding.binding = 0;
        textureBinding.textureView = textureView;
        
        wgpu::BindGroupEntry samplerBinding(wgpu::Default);
        samplerBinding.binding = 1;
        samplerBinding.sampler = sampler;
        
        std::array<wgpu::BindGroupEntry, 2> bindings = { textureBinding, samplerBinding };
        
        wgpu::BindGroupDescriptor bindGroupDesc(wgpu::Default);
        bindGroupDesc.layout = _textureBindGroupLayout;
        bindGroupDesc.entryCount = bindings.size();
        bindGroupDesc.entries = bindings.data();
        bindGroupDesc.label = wgpu::StringView("RmlUI Texture Bind Group");
        
        auto bindGroup = device.createBindGroup(bindGroupDesc);
        
        auto rmlTexture = std::make_unique<RmlUITexture>();
        rmlTexture->texture = texture;
        rmlTexture->textureView = textureView;
        rmlTexture->sampler = sampler;
        rmlTexture->bindGroup = bindGroup;
        rmlTexture->dimensions = texture_dimensions;
        
        _textures[handle] = std::move(rmlTexture);
        
        return handle;
    }

    Rml::TextureHandle WGPURenderInterface::GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i dimensions)
    {
        auto handle = _nextTextureHandle++;
        auto device = _core.GetResource<wgpu::Device>();
        auto queue = _core.GetResource<wgpu::Queue>();
        
        wgpu::TextureDescriptor textureDesc(wgpu::Default);
        textureDesc.label = wgpu::StringView("RmlUI Generated Texture");
        textureDesc.size = { static_cast<uint32_t>(dimensions.x), static_cast<uint32_t>(dimensions.y), 1 };
        textureDesc.format = wgpu::TextureFormat::RGBA8Unorm;
        textureDesc.usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst;
        textureDesc.dimension = wgpu::TextureDimension::_2D;
        
        auto texture = device.createTexture(textureDesc);
        
        wgpu::Extent3D textureSize = { static_cast<uint32_t>(dimensions.x), static_cast<uint32_t>(dimensions.y), 1 };
        wgpu::TexelCopyTextureInfo destination;
        destination.texture = texture;
        destination.mipLevel = 0;
        destination.origin = { 0, 0, 0 };
        destination.aspect = wgpu::TextureAspect::All;
        
        wgpu::TexelCopyBufferLayout sourceLayout;
        sourceLayout.offset = 0;
        sourceLayout.bytesPerRow = 4 * dimensions.x;
        sourceLayout.rowsPerImage = dimensions.y;
        
        queue.writeTexture(destination, source.data(), 4 * dimensions.x * dimensions.y, sourceLayout, textureSize);
        
        wgpu::TextureViewDescriptor textureViewDesc(wgpu::Default);
        textureViewDesc.format = wgpu::TextureFormat::RGBA8Unorm;
        textureViewDesc.dimension = wgpu::TextureViewDimension::_2D;
        textureViewDesc.baseMipLevel = 0;
        textureViewDesc.mipLevelCount = 1;
        textureViewDesc.baseArrayLayer = 0;
        textureViewDesc.arrayLayerCount = 1;
        
        auto textureView = texture.createView(textureViewDesc);
        
        wgpu::SamplerDescriptor samplerDesc(wgpu::Default);
        samplerDesc.magFilter = wgpu::FilterMode::Linear;
        samplerDesc.minFilter = wgpu::FilterMode::Linear;
        samplerDesc.mipmapFilter = wgpu::MipmapFilterMode::Linear;
        samplerDesc.addressModeU = wgpu::AddressMode::ClampToEdge;
        samplerDesc.addressModeV = wgpu::AddressMode::ClampToEdge;
        samplerDesc.addressModeW = wgpu::AddressMode::ClampToEdge;
        samplerDesc.maxAnisotropy = 1;
        
        auto sampler = device.createSampler(samplerDesc);
        
        wgpu::BindGroupEntry textureBinding(wgpu::Default);
        textureBinding.binding = 0;
        textureBinding.textureView = textureView;
        
        wgpu::BindGroupEntry samplerBinding(wgpu::Default);
        samplerBinding.binding = 1;
        samplerBinding.sampler = sampler;
        
        std::array<wgpu::BindGroupEntry, 2> bindings = { textureBinding, samplerBinding };
        
        wgpu::BindGroupDescriptor bindGroupDesc(wgpu::Default);
        bindGroupDesc.layout = _textureBindGroupLayout;
        bindGroupDesc.entryCount = bindings.size();
        bindGroupDesc.entries = bindings.data();
        bindGroupDesc.label = wgpu::StringView("RmlUI Generated Texture Bind Group");
        
        auto bindGroup = device.createBindGroup(bindGroupDesc);
        
        auto rmlTexture = std::make_unique<RmlUITexture>();
        rmlTexture->texture = texture;
        rmlTexture->textureView = textureView;
        rmlTexture->sampler = sampler;
        rmlTexture->bindGroup = bindGroup;
        rmlTexture->dimensions = dimensions;
        
        _textures[handle] = std::move(rmlTexture);
        
        return handle;
    }

    Rml::TextureHandle WGPURenderInterface::CreateTexture(Rml::Span<const Rml::byte> source_data, Rml::Vector2i source_dimensions)
    {
        return GenerateTexture(source_data, source_dimensions);
    }

    void WGPURenderInterface::ReleaseTexture(Rml::TextureHandle handle)
    {
        auto it = _textures.find(handle);
        if (it != _textures.end()) {
            it->second->texture.release();
            it->second->textureView.release();
            it->second->sampler.release();
            it->second->bindGroup.release();
            _textures.erase(it);
        }
    }

    void WGPURenderInterface::EnableScissorRegion(bool enable)
    {
        _scissorEnabled = enable;
    }

    void WGPURenderInterface::SetScissorRegion(Rml::Rectanglei region)
    {
        _scissorRegion = region;
    }

    void WGPURenderInterface::SetScissor(Rml::Rectanglei region, bool vertically_flip)
    {
        _scissorRegion = region;
        // Note: vertical flip handling would need to be implemented in the shader
        // or by adjusting the transform matrix
    }

    void WGPURenderInterface::BeginFrame()
    {
        // Reset any per-frame state if needed
    }

    void WGPURenderInterface::EndFrame()
    {
        // Clean up any per-frame resources if needed
    }

    void WGPURenderInterface::DrawFullscreenQuad()
    {
        // This would draw a fullscreen quad for post-processing effects
        // Implementation depends on specific requirements
    }

    void WGPURenderInterface::SetTransform(const Rml::Matrix4f *new_transform)
    {
        if (new_transform) {
            _transform = *new_transform;
        } else {
            _transform = Rml::Matrix4f::Identity();
        }
    }

    void WGPURenderInterface::SetRenderPassEncoder(wgpu::RenderPassEncoder renderPass)
    {
        _currentRenderPass = renderPass;
    }

    void WGPURenderInterface::RenderQueuedGeometry()
    {
        if (!_currentRenderPass) return;
        
        auto device = _core.GetResource<wgpu::Device>();
        auto queue = _core.GetResource<wgpu::Queue>();
        
        _currentRenderPass.setPipeline(_uiPipeline);
        _currentRenderPass.setBindGroup(0, _uniformBindGroup, 0, nullptr);
        
        for (const auto& queued : _renderQueue) {
            auto it = _compiledGeometries.find(queued.handle);
            if (it == _compiledGeometries.end()) continue;
            
            auto& geometry = it->second;
            
            glm::mat4 transform = glm::mat4(1.0f);
            transform = glm::translate(transform, glm::vec3(queued.translation.x, queued.translation.y, 0.0f));
            
            glm::mat4 rmlTransform;
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    rmlTransform[i][j] = _transform[i][j];
                }
            }
            transform = rmlTransform * transform;
            
            queue.writeBuffer(_uniformBuffer, 0, &transform, sizeof(glm::mat4));
            
            _currentRenderPass.setVertexBuffer(0, geometry->vertexBuffer, 0, geometry->vertexCount * sizeof(RmlUIVertex));
            _currentRenderPass.setIndexBuffer(geometry->indexBuffer, wgpu::IndexFormat::Uint32, 0, geometry->indexCount * sizeof(uint32_t));
            
            // Set texture bind group (either specific texture or default white texture)
            if (queued.textureHandle != 0) {
                auto* texture = GetTexture(queued.textureHandle);
                if (texture) {
                    _currentRenderPass.setBindGroup(1, texture->bindGroup, 0, nullptr);
                } else {
                    _currentRenderPass.setBindGroup(1, _defaultTextureBindGroup, 0, nullptr);
                }
            } else {
                _currentRenderPass.setBindGroup(1, _defaultTextureBindGroup, 0, nullptr);
            }
            
            if (_scissorEnabled) {
                // TODO: Fix Rectangle member access - need to check RmlUI Rectangle structure
                // _currentRenderPass.setScissorRect(_scissorRegion.origin.x, _scissorRegion.origin.y, 
                //                                 _scissorRegion.size.x, _scissorRegion.size.y);
            }
            
            _currentRenderPass.drawIndexed(geometry->indexCount, 1, 0, 0, 0);
        }
        
        _renderQueue.clear();
    }
}
