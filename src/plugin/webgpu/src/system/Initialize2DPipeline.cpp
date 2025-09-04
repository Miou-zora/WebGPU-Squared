#include "Initialize2DPipeline.hpp"
#include "WebGPU.hpp"
#include "resource/window/Window.hpp"
#include "utils.hpp"
#include "structs.hpp"

namespace ES::Plugin::WebGPU::System {


void Initialize2DPipeline(ES::Engine::Core &core)
{
	auto &device = core.GetResource<wgpu::Device>();
	auto &surface = core.GetResource<wgpu::Surface>();
	auto &window = core.GetResource<ES::Plugin::Window::Resource::Window>();
	wgpu::TextureFormat surfaceFormat = core.GetResource<wgpu::SurfaceCapabilities>().formats[0];


	if (device == nullptr) throw std::runtime_error("WebGPU device is not created, cannot initialize 2D pipeline.");
	if (surface == nullptr) throw std::runtime_error("WebGPU surface is not created, cannot initialize 2D pipeline.");

	wgpu::RenderPipelineDescriptor pipelineDesc(wgpu::Default);
	pipelineDesc.label = wgpu::StringView("2D Render Pipeline");

	wgpu::ShaderSourceWGSL wgslDesc(wgpu::Default);
	std::string wgslSource = loadFile("shader2D.wgsl");
	wgslDesc.code = wgpu::StringView(wgslSource);
	wgpu::ShaderModuleDescriptor shaderDesc(wgpu::Default);
    shaderDesc.nextInChain = &wgslDesc.chain; // connect the chained extension
    shaderDesc.label = wgpu::StringView("Shader source from Application.cpp");
	wgpu::ShaderModule shaderModule = device.createShaderModule(shaderDesc);
	wgpu::VertexBufferLayout vertexBufferLayout(wgpu::Default);

	std::vector<wgpu::VertexAttribute> vertexAttribs(3);

    // Describe the position attribute
    vertexAttribs[0].shaderLocation = 0;
    vertexAttribs[0].format = wgpu::VertexFormat::Float32x3;
    vertexAttribs[0].offset = 0;

	vertexAttribs[1].shaderLocation = 1;
	vertexAttribs[1].format = wgpu::VertexFormat::Float32x3;
	vertexAttribs[1].offset = 3 * sizeof(float); // Offset by the size of the position attribute

	vertexAttribs[2].shaderLocation = 2;
	vertexAttribs[2].format = wgpu::VertexFormat::Float32x2;
	vertexAttribs[2].offset = 6 * sizeof(float); // Offset by the size of the position attribute

    vertexBufferLayout.attributeCount = static_cast<uint32_t>(vertexAttribs.size());
    vertexBufferLayout.attributes = vertexAttribs.data();

    vertexBufferLayout.arrayStride = (8 * sizeof(float));
    vertexBufferLayout.stepMode = wgpu::VertexStepMode::Vertex;

		// TODO: find why it does not work with wgpu::BindGroupLayoutEntry
	WGPUBindGroupLayoutEntry bindingLayout = {0};
	bindingLayout.binding = 0;
	bindingLayout.visibility = wgpu::ShaderStage::Vertex;
	bindingLayout.buffer.type = wgpu::BufferBindingType::Uniform;
	bindingLayout.buffer.minBindingSize = sizeof(Uniforms2D);

	std::array<WGPUBindGroupLayoutEntry, 1> uniformsBindings = { bindingLayout };

	wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc(wgpu::Default);
	bindGroupLayoutDesc.entryCount = uniformsBindings.size();
	bindGroupLayoutDesc.entries = uniformsBindings.data();
	bindGroupLayoutDesc.label = wgpu::StringView("Uniforms Bind Group Layout");
	wgpu::BindGroupLayout uniformsBindGroupLayout = device.createBindGroupLayout(bindGroupLayoutDesc);

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

	std::array<WGPUBindGroupLayout, 2> bindGroupLayouts = { uniformsBindGroupLayout, textureBindGroupLayout };

	wgpu::PipelineLayoutDescriptor layoutDesc(wgpu::Default);
	layoutDesc.bindGroupLayoutCount = bindGroupLayouts.size();
	layoutDesc.bindGroupLayouts = bindGroupLayouts.data();
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

	int frameBufferSizeX, frameBufferSizeY;
	glfwGetFramebufferSize(window.GetGLFWWindow(), &frameBufferSizeX, &frameBufferSizeY);

	wgpu::DepthStencilState depthStencilState(wgpu::Default);
	depthStencilState.depthCompare = wgpu::CompareFunction::Less;
	depthStencilState.depthWriteEnabled = wgpu::OptionalBool::True;
	depthStencilState.format = depthTextureFormat;
	pipelineDesc.depthStencil = &depthStencilState;

	wgpu::RenderPipeline pipeline = device.createRenderPipeline(pipelineDesc);

	if (pipeline == nullptr) throw std::runtime_error("Could not create render pipeline");

	wgpuShaderModuleRelease(shaderModule);

	core.GetResource<Pipelines>().renderPipelines["2D"] = PipelineData{
		.pipeline = pipeline,
		.bindGroupLayouts = {
			uniformsBindGroupLayout,
			textureBindGroupLayout
		},
		.layout = layout,
	};
}

}
