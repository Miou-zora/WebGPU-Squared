#include "InitializeDeferredPipeline.hpp"
#include "WebGPU.hpp"
#include "Window.hpp"

namespace ES::Plugin::WebGPU::System {

void InitializeDeferredPipeline(ES::Engine::Core &core)
{
	wgpu::Device device = core.GetResource<wgpu::Device>();
	wgpu::TextureFormat surfaceFormat = core.GetResource<wgpu::SurfaceCapabilities>().formats[0];

	if (device == nullptr) throw std::runtime_error("WebGPU device is not created, cannot initialize pipeline.");

	wgpu::ShaderSourceWGSL wgslDesc(wgpu::Default);
	std::string wgslSource = loadFile("shaderDeferred.wgsl");
	wgslDesc.code = wgpu::StringView(wgslSource);

	wgpu::ShaderModuleDescriptor shaderDesc(wgpu::Default);
    shaderDesc.nextInChain = &wgslDesc.chain; // connect the chained extension
    shaderDesc.label = wgpu::StringView("Shader source from Application.cpp");

	wgpu::ShaderModule shaderModule = device.createShaderModule(shaderDesc);
	wgpu::RenderPipelineDescriptor pipelineDesc(wgpu::Default);
	pipelineDesc.label = wgpu::StringView("My Deferred Render Pipeline");
	wgpu::VertexBufferLayout vertexBufferLayout(wgpu::Default);

	std::vector<wgpu::VertexAttribute> vertexAttribs(0);

    vertexBufferLayout.attributeCount = static_cast<uint32_t>(vertexAttribs.size());
    vertexBufferLayout.attributes = vertexAttribs.data();

    vertexBufferLayout.arrayStride = 0;
    vertexBufferLayout.stepMode = wgpu::VertexStepMode::Vertex;

	// TODO: find why it does not work with wgpu::BindGroupLayoutEntry
	// NORMAL
	WGPUBindGroupLayoutEntry bindingLayoutNormal = {0};
	bindingLayoutNormal.binding = 0;
	bindingLayoutNormal.visibility = wgpu::ShaderStage::Fragment;
	bindingLayoutNormal.texture.sampleType = wgpu::TextureSampleType::UnfilterableFloat;
	bindingLayoutNormal.texture.viewDimension = wgpu::TextureViewDimension::_2D;

	// ALBEDO
	WGPUBindGroupLayoutEntry bindingLayoutAlbedo = {0};
	bindingLayoutAlbedo.binding = 1;
	bindingLayoutAlbedo.visibility = wgpu::ShaderStage::Fragment;
	bindingLayoutAlbedo.texture.sampleType = wgpu::TextureSampleType::UnfilterableFloat;
	bindingLayoutAlbedo.texture.viewDimension = wgpu::TextureViewDimension::_2D;

	// DEPTH
	WGPUBindGroupLayoutEntry bindingLayoutDepth = {0};
	bindingLayoutDepth.binding = 2;
	bindingLayoutDepth.visibility = wgpu::ShaderStage::Fragment;
	bindingLayoutDepth.texture.sampleType = wgpu::TextureSampleType::UnfilterableFloat;
	bindingLayoutDepth.texture.viewDimension = wgpu::TextureViewDimension::_2D;


	std::array<WGPUBindGroupLayoutEntry, 3> bindings = { bindingLayoutNormal, bindingLayoutAlbedo, bindingLayoutDepth };

	wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc(wgpu::Default);
	bindGroupLayoutDesc.entryCount = bindings.size();
	bindGroupLayoutDesc.entries = bindings.data();
	bindGroupLayoutDesc.label = wgpu::StringView("My Bind Group Layout");
	wgpu::BindGroupLayout bindGroupLayout = device.createBindGroupLayout(bindGroupLayoutDesc);

	WGPUBindGroupLayoutEntry bindingLayoutLights = {0};
	bindingLayoutLights.binding = 0;
	bindingLayoutLights.visibility = wgpu::ShaderStage::Fragment;
	bindingLayoutLights.buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;
	bindingLayoutLights.buffer.minBindingSize = sizeof(uint32_t) + 12 /* (padding) */ + sizeof(Light);

	std::array<WGPUBindGroupLayoutEntry, 1> bindingsLights = { bindingLayoutLights };

	wgpu::BindGroupLayoutDescriptor bindGroupLayoutDescLights(wgpu::Default);
	bindGroupLayoutDescLights.entryCount = bindingsLights.size();
	bindGroupLayoutDescLights.entries = bindingsLights.data();
	bindGroupLayoutDescLights.label = wgpu::StringView("Lights Bind Group Layout");
	wgpu::BindGroupLayout bindGroupLayoutLights = device.createBindGroupLayout(bindGroupLayoutDescLights);

	WGPUBindGroupLayoutEntry bindingLayoutCamera = {0};
	bindingLayoutCamera.binding = 0;
	bindingLayoutCamera.visibility = wgpu::ShaderStage::Fragment;
	bindingLayoutCamera.buffer.type = wgpu::BufferBindingType::Uniform;
	bindingLayoutCamera.buffer.minBindingSize = sizeof(glm::mat4) * 2 + sizeof(glm::vec3) + sizeof(float);

	std::array<WGPUBindGroupLayoutEntry, 1> bindingsCamera = { bindingLayoutCamera };

	wgpu::BindGroupLayoutDescriptor bindGroupLayoutDescCamera(wgpu::Default);
	bindGroupLayoutDescCamera.entryCount = bindingsCamera.size();
	bindGroupLayoutDescCamera.entries = bindingsCamera.data();
	bindGroupLayoutDescCamera.label = wgpu::StringView("Camera Bind Group Layout");
	wgpu::BindGroupLayout bindGroupLayoutCamera = device.createBindGroupLayout(bindGroupLayoutDescCamera);

	std::array<WGPUBindGroupLayout, 3> bindGroupLayouts = {bindGroupLayout, bindGroupLayoutLights, bindGroupLayoutCamera};

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

	shaderModule.release();

	core.GetResource<Pipelines>().renderPipelines["Deferred"] = PipelineData{
		.pipeline = pipeline,
		.bindGroupLayouts = {bindGroupLayout, bindGroupLayoutLights, bindGroupLayoutCamera},
		.layout = layout,
	};
}
}
