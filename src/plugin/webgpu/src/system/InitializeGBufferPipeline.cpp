#include "InitializeGBufferPipeline.hpp"
#include "structs.hpp"
#include "WebGPU.hpp"
#include "resource/window/Window.hpp"
#include "utils.hpp"
#include <fmt/core.h>

namespace ES::Plugin::WebGPU::System {
void InitializeGBufferPipeline(ES::Engine::Core &core) {
    auto &device = core.GetResource<wgpu::Device>();
    auto &surface = core.GetResource<wgpu::Surface>();

    if (device == nullptr) throw std::runtime_error("WebGPU device is not created, cannot initialize GBuffer pipeline.");
    if (surface == nullptr) throw std::runtime_error("WebGPU surface is not created, cannot initialize GBuffer pipeline.");

    const std::string pipelineName = "GBuffer";

    wgpu::RenderPipelineDescriptor pipelineDesc(wgpu::Default);
    pipelineDesc.label = wgpu::StringView(fmt::format("{} Pipeline", pipelineName));

    wgpu::ShaderSourceWGSL wgslDesc(wgpu::Default);
    std::string wgslSource = loadFile("./assets/shader/shaderGBuffer.wgsl");
    wgslDesc.code = wgpu::StringView(wgslSource);
    wgpu::ShaderModuleDescriptor shaderDesc(wgpu::Default);
    shaderDesc.nextInChain = &wgslDesc.chain;
    shaderDesc.label = wgpu::StringView("Shader source GBuffer");
    wgpu::ShaderModule shaderModule = device.createShaderModule(shaderDesc);

    std::vector<wgpu::VertexAttribute> vertexAttribs(3);
    vertexAttribs[0].shaderLocation = 0;
    vertexAttribs[0].offset = 0;
    vertexAttribs[0].format = wgpu::VertexFormat::Float32x3;
    vertexAttribs[1].shaderLocation = 1;
    vertexAttribs[1].offset = 3 * sizeof(float);
    vertexAttribs[1].format = wgpu::VertexFormat::Float32x3;
    vertexAttribs[2].shaderLocation = 2;
    vertexAttribs[2].offset = 6 * sizeof(float);
    vertexAttribs[2].format = wgpu::VertexFormat::Float32x2;

    wgpu::VertexBufferLayout vertexBufferLayout(wgpu::Default);
    vertexBufferLayout.attributeCount = static_cast<uint32_t>(vertexAttribs.size());
    vertexBufferLayout.attributes = vertexAttribs.data();
    vertexBufferLayout.arrayStride = (8 * sizeof(float));
    vertexBufferLayout.stepMode = wgpu::VertexStepMode::Vertex;

    std::vector<wgpu::VertexAttribute> vertexAttribsUniformsIndex(1);
    vertexAttribsUniformsIndex[0].shaderLocation = 3;
    vertexAttribsUniformsIndex[0].offset = 0;
    vertexAttribsUniformsIndex[0].format = wgpu::VertexFormat::Uint32;

    wgpu::VertexBufferLayout vertexBufferLayoutUniformsIndex(wgpu::Default);
    vertexBufferLayoutUniformsIndex.attributeCount = static_cast<uint32_t>(vertexAttribsUniformsIndex.size());
    vertexBufferLayoutUniformsIndex.attributes = vertexAttribsUniformsIndex.data();
    vertexBufferLayoutUniformsIndex.arrayStride = sizeof(uint32_t);
    vertexBufferLayoutUniformsIndex.stepMode = wgpu::VertexStepMode::Instance;

    WGPUBindGroupLayoutEntry bindingLayoutCamera = {0};
    bindingLayoutCamera.binding = 0;
    bindingLayoutCamera.visibility = wgpu::ShaderStage::Vertex;
    bindingLayoutCamera.buffer.type = wgpu::BufferBindingType::Uniform;
    bindingLayoutCamera.buffer.minBindingSize = sizeof(glm::mat4) * 2 + sizeof(glm::vec3) + sizeof(float);

    std::array<WGPUBindGroupLayoutEntry, 1> cameraBindings = { bindingLayoutCamera };

    wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc(wgpu::Default);
    bindGroupLayoutDesc.entryCount = cameraBindings.size();
    bindGroupLayoutDesc.entries = cameraBindings.data();
    bindGroupLayoutDesc.label = wgpu::StringView("Camera Bind Group Layout");
    wgpu::BindGroupLayout cameraBindGroupLayout = device.createBindGroupLayout(bindGroupLayoutDesc);

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

    WGPUBindGroupLayoutEntry bindingLayoutUniforms = {0};
    bindingLayoutUniforms.binding = 0;
    bindingLayoutUniforms.visibility = wgpu::ShaderStage::Vertex;
    bindingLayoutUniforms.buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;
    bindingLayoutUniforms.buffer.minBindingSize = sizeof(glm::mat4) * 2;

    std::array<WGPUBindGroupLayoutEntry, 1> uniformsBindings = { bindingLayoutUniforms };

    bindGroupLayoutDesc.entryCount = uniformsBindings.size();
    bindGroupLayoutDesc.entries = uniformsBindings.data();
    bindGroupLayoutDesc.label = wgpu::StringView("Uniforms Bind Group Layout");
    wgpu::BindGroupLayout uniformsBindGroupLayout = device.createBindGroupLayout(bindGroupLayoutDesc);

    std::array<WGPUBindGroupLayout, 3> bindGroupLayouts = { cameraBindGroupLayout, textureBindGroupLayout, uniformsBindGroupLayout };

    wgpu::PipelineLayoutDescriptor layoutDesc(wgpu::Default);
    layoutDesc.bindGroupLayoutCount = bindGroupLayouts.size();
    layoutDesc.bindGroupLayouts = bindGroupLayouts.data();
    wgpu::PipelineLayout layout = device.createPipelineLayout(layoutDesc);

    std::array<WGPUVertexBufferLayout, 2> vertexBuffers = { vertexBufferLayout, vertexBufferLayoutUniformsIndex };

    pipelineDesc.vertex.bufferCount = vertexBuffers.size();
    pipelineDesc.vertex.buffers = vertexBuffers.data();
    pipelineDesc.vertex.module = shaderModule;
    pipelineDesc.vertex.entryPoint = wgpu::StringView("vs_main");

    wgpu::FragmentState fragmentState(wgpu::Default);
    fragmentState.module = shaderModule;
    fragmentState.entryPoint = wgpu::StringView("fs_main");

    wgpu::BlendState blendState(wgpu::Default);
    std::vector<wgpu::ColorTargetState> colorTargets;

    wgpu::ColorTargetState normalColorTarget(wgpu::Default);
    normalColorTarget.format = wgpu::TextureFormat::RGBA16Float;
    normalColorTarget.writeMask = wgpu::ColorWriteMask::All;
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
        .bindGroupLayouts = { cameraBindGroupLayout, textureBindGroupLayout, uniformsBindGroupLayout },
        .layout = layout,
    };
}
}
