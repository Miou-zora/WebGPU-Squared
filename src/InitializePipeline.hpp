#pragma once

#include "utils.hpp"
#include "Engine.hpp"
#include <webgpu/webgpu.h>

void InitializePipeline(ES::Engine::Core &core)
{
	wgpu::Device device = core.GetResource<wgpu::Device>();
	wgpu::TextureFormat surfaceFormat = core.GetResource<wgpu::SurfaceCapabilities>().formats[0];

	if (device == nullptr) throw std::runtime_error("WebGPU device is not created, cannot initialize pipeline.");

	wgpu::ShaderSourceWGSL wgslDesc(wgpu::Default);
	std::string wgslSource = loadFile("shader.wgsl");
	wgslDesc.code = wgpu::StringView(wgslSource);

	wgpu::ShaderModuleDescriptor shaderDesc(wgpu::Default);
    shaderDesc.nextInChain = &wgslDesc.chain; // connect the chained extension
    shaderDesc.label = wgpu::StringView("Shader source from Application.cpp");

	wgpu::ShaderModule shaderModule = device.createShaderModule(shaderDesc);
	wgpu::RenderPipelineDescriptor pipelineDesc(wgpu::Default);
	pipelineDesc.label = wgpu::StringView("My Render Pipeline");
	wgpu::VertexBufferLayout vertexBufferLayout(wgpu::Default);

	std::vector<wgpu::VertexAttribute> vertexAttribs(2);

    // Describe the position attribute
    vertexAttribs[0].shaderLocation = 0;
    vertexAttribs[0].format = wgpu::VertexFormat::Float32x3;
    vertexAttribs[0].offset = 0;

    // Describe the color attribute
    vertexAttribs[1].shaderLocation = 1;
    vertexAttribs[1].format = wgpu::VertexFormat::Float32x3;
    vertexAttribs[1].offset = 3 * sizeof(float);

    vertexBufferLayout.attributeCount = static_cast<uint32_t>(vertexAttribs.size());
    vertexBufferLayout.attributes = vertexAttribs.data();

    vertexBufferLayout.arrayStride = 6 * sizeof(float);
    vertexBufferLayout.stepMode = wgpu::VertexStepMode::Vertex;

	// TODO: find why it does not work with wgpu::BindGroupLayoutEntry
	WGPUBindGroupLayoutEntry bindingLayout = {0};
	bindingLayout.binding = 0;
	bindingLayout.visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
	bindingLayout.buffer.type = wgpu::BufferBindingType::Uniform;
	bindingLayout.buffer.minBindingSize = sizeof(MyUniforms);

	WGPUBindGroupLayoutEntry bindingLayout2 = {0};
	bindingLayout2.binding = 1;
	bindingLayout2.visibility = wgpu::ShaderStage::Fragment;
	bindingLayout2.buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;
	bindingLayout2.buffer.minBindingSize = sizeof(Light) * MAX_LIGHTS;

	std::array<WGPUBindGroupLayoutEntry, 2> bindings = { bindingLayout, bindingLayout2 };

	wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc(wgpu::Default);
	bindGroupLayoutDesc.entryCount = 2;
	bindGroupLayoutDesc.entries = bindings.data();
	bindGroupLayoutDesc.label = wgpu::StringView("My Bind Group Layout");
	wgpu::BindGroupLayout bindGroupLayout = device.createBindGroupLayout(bindGroupLayoutDesc);

	wgpu::PipelineLayoutDescriptor layoutDesc(wgpu::Default);
	layoutDesc.bindGroupLayoutCount = 1;
	layoutDesc.bindGroupLayouts = reinterpret_cast<WGPUBindGroupLayout*>(&bindGroupLayout);
	wgpu::PipelineLayout layout = device.createPipelineLayout(layoutDesc);

    pipelineDesc.vertex.bufferCount = 1;
    pipelineDesc.vertex.buffers = &vertexBufferLayout;
	pipelineDesc.vertex.module = shaderModule;
    pipelineDesc.vertex.entryPoint = wgpu::StringView("vs_main");

	wgpu::FragmentState fragmentState(wgpu::Default);
    fragmentState.module = shaderModule;
	fragmentState.entryPoint = wgpu::StringView("fs_main");

	wgpu::ColorTargetState colorTarget(wgpu::Default);
    colorTarget.format = surfaceFormat;
	colorTarget.writeMask = wgpu::ColorWriteMask::All;

	wgpu::BlendState blendState(wgpu::Default);
    colorTarget.blend = &blendState;
    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTarget;
    pipelineDesc.fragment = &fragmentState;
	pipelineDesc.layout = layout;

	auto &window = core.GetResource<ES::Plugin::Window::Resource::Window>();

	int frameBufferSizeX, frameBufferSizeY;
	glfwGetFramebufferSize(window.GetGLFWWindow(), &frameBufferSizeX, &frameBufferSizeY);

	wgpu::DepthStencilState depthStencilState(wgpu::Default);
	depthStencilState.depthCompare = wgpu::CompareFunction::Less;
	depthStencilState.depthWriteEnabled = wgpu::OptionalBool::True;
	depthStencilState.format = depthTextureFormat;
	pipelineDesc.depthStencil = &depthStencilState;

	// TODO: Use async pipeline creation
	wgpu::RenderPipeline pipeline = device.createRenderPipeline(pipelineDesc);

	if (pipeline == nullptr) throw std::runtime_error("Could not create render pipeline");

	wgpuShaderModuleRelease(shaderModule);

	core.GetResource<Pipelines>().renderPipelines["3D"] = PipelineData{
		.pipeline = pipeline,
		.bindGroupLayouts = {bindGroupLayout},
		.layout = layout,
	};
}
