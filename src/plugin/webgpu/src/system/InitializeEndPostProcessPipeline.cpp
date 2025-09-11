#include "InitializeEndPostProcessPipeline.hpp"
#include "WebGPU.hpp"
#include "resource/Window/Window.hpp"

namespace ES::Plugin::WebGPU::System {

void InitializeEndPostProcessPipeline(ES::Engine::Core &core)
{
	wgpu::Device device = core.GetResource<wgpu::Device>();
	wgpu::TextureFormat surfaceFormat = core.GetResource<wgpu::SurfaceCapabilities>().formats[0];

	wgpu::ShaderSourceWGSL wgslDesc(wgpu::Default);
	std::string wgslSource = loadFile("shaderEndPostProcess.wgsl");
	wgslDesc.code = wgpu::StringView(wgslSource);

	wgpu::ShaderModuleDescriptor shaderDesc(wgpu::Default);
    shaderDesc.nextInChain = &wgslDesc.chain; // connect the chained extension
    shaderDesc.label = wgpu::StringView("Shader source end post process");

	wgpu::ShaderModule shaderModule = device.createShaderModule(shaderDesc);
	wgpu::RenderPipelineDescriptor pipelineDesc(wgpu::Default);
	pipelineDesc.label = wgpu::StringView("My End Post Process Pipeline");
	wgpu::VertexBufferLayout vertexBufferLayout(wgpu::Default);

	std::vector<wgpu::VertexAttribute> vertexAttributes(0);

    vertexBufferLayout.attributeCount = static_cast<uint32_t>(vertexAttributes.size());
    vertexBufferLayout.attributes = vertexAttributes.data();

    vertexBufferLayout.arrayStride = 0;
    vertexBufferLayout.stepMode = wgpu::VertexStepMode::Vertex;

	// TODO: find why it does not work with wgpu::BindGroupLayoutEntry
	WGPUBindGroupLayoutEntry bindingLayoutNormal = {0};
	bindingLayoutNormal.binding = 0;
	bindingLayoutNormal.visibility = wgpu::ShaderStage::Fragment;
	bindingLayoutNormal.texture.sampleType = wgpu::TextureSampleType::UnfilterableFloat;
	bindingLayoutNormal.texture.viewDimension = wgpu::TextureViewDimension::_2D;

	std::array<WGPUBindGroupLayoutEntry, 1> bindings = { bindingLayoutNormal };

	wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc(wgpu::Default);
	bindGroupLayoutDesc.entryCount = bindings.size();
	bindGroupLayoutDesc.entries = bindings.data();
	bindGroupLayoutDesc.label = wgpu::StringView("Input Texture Bind Group Layout");
	wgpu::BindGroupLayout bindGroupLayout = device.createBindGroupLayout(bindGroupLayoutDesc);

	std::array<WGPUBindGroupLayout, 1> bindGroupLayouts = {bindGroupLayout};

	wgpu::PipelineLayoutDescriptor layoutDesc(wgpu::Default);
	layoutDesc.bindGroupLayoutCount = bindGroupLayouts.size();
	layoutDesc.bindGroupLayouts = bindGroupLayouts.data();
	wgpu::PipelineLayout layout = device.createPipelineLayout(layoutDesc);

    pipelineDesc.vertex.bufferCount = 0;
    pipelineDesc.vertex.buffers = &vertexBufferLayout;
	pipelineDesc.vertex.module = shaderModule;
    pipelineDesc.vertex.entryPoint = wgpu::StringView("vs_main");

	wgpu::FragmentState fragmentState(wgpu::Default);
    fragmentState.module = shaderModule;
	fragmentState.entryPoint = wgpu::StringView("fs_main");

	std::vector<wgpu::ColorTargetState> colorTargets(0);
	wgpu::ColorTargetState colorTarget(wgpu::Default);
    colorTarget.format = surfaceFormat;
	colorTarget.writeMask = wgpu::ColorWriteMask::All;
	colorTargets.push_back(colorTarget);

	wgpu::BlendState blendState(wgpu::Default);
    colorTarget.blend = &blendState;
    fragmentState.targetCount = static_cast<uint32_t>(colorTargets.size());
    fragmentState.targets = colorTargets.data();
    pipelineDesc.fragment = &fragmentState;
	pipelineDesc.layout = layout;

	pipelineDesc.depthStencil = nullptr;

	// TODO: Use async pipeline creation
	wgpu::RenderPipeline pipeline = device.createRenderPipeline(pipelineDesc);

	if (pipeline == nullptr) throw std::runtime_error("Could not create render pipeline");

	shaderModule.release();

	core.GetResource<Pipelines>().renderPipelines["EndPostProcess"] = PipelineData{
		.pipeline = pipeline,
		.bindGroupLayouts = {bindGroupLayout},
		.layout = layout,
	};
}
}
