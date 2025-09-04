#include "InitializeShadowPipeline.hpp"
#include "WebGPU.hpp"
#include "resource/window/Window.hpp"

namespace ES::Plugin::WebGPU::System {

void InitializeShadowPipeline(ES::Engine::Core &core)
{
	const std::string passName = "ShadowPass";
	wgpu::Device device = core.GetResource<wgpu::Device>();
	wgpu::TextureFormat surfaceFormat = core.GetResource<wgpu::SurfaceCapabilities>().formats[0];

	if (device == nullptr) throw std::runtime_error("WebGPU device is not created, cannot initialize pipeline.");

	wgpu::ShaderSourceWGSL wgslDesc(wgpu::Default);
	std::string wgslSource = loadFile("shaderShadow.wgsl");
	wgslDesc.code = wgpu::StringView(wgslSource);

	wgpu::ShaderModuleDescriptor shaderDesc(wgpu::Default);
    shaderDesc.nextInChain = &wgslDesc.chain; // connect the chained extension
    shaderDesc.label = wgpu::StringView("Shader source from Application.cpp");

	wgpu::ShaderModule shaderModule = device.createShaderModule(shaderDesc);
	wgpu::RenderPipelineDescriptor pipelineDesc(wgpu::Default);
	pipelineDesc.label = wgpu::StringView("My Shadow Render Pipeline");

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

    std::vector<wgpu::VertexAttribute> vertexAttribsTransformsIndex(1);
    vertexAttribsTransformsIndex[0].shaderLocation = 3;
    vertexAttribsTransformsIndex[0].offset = 0;
    vertexAttribsTransformsIndex[0].format = wgpu::VertexFormat::Uint32;

	wgpu::VertexBufferLayout vertexBufferLayoutTransformsIndex(wgpu::Default);
    vertexBufferLayoutTransformsIndex.attributeCount = static_cast<uint32_t>(vertexAttribsTransformsIndex.size());
    vertexBufferLayoutTransformsIndex.attributes = vertexAttribsTransformsIndex.data();
    vertexBufferLayoutTransformsIndex.arrayStride = (8 * sizeof(float));
    vertexBufferLayoutTransformsIndex.stepMode = wgpu::VertexStepMode::Vertex;




    WGPUBindGroupLayoutEntry transformsBindingLayout = {0};
    transformsBindingLayout.binding = 0;
    transformsBindingLayout.visibility = wgpu::ShaderStage::Vertex;
    transformsBindingLayout.buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;
    transformsBindingLayout.buffer.minBindingSize = sizeof(glm::mat4) * 2;

    std::array<WGPUBindGroupLayoutEntry, 1> transformsBindings = { transformsBindingLayout };

	wgpu::BindGroupLayoutDescriptor transformBindGroupLayoutDesc(wgpu::Default);
    transformBindGroupLayoutDesc.entryCount = transformsBindings.size();
    transformBindGroupLayoutDesc.entries = transformsBindings.data();
    transformBindGroupLayoutDesc.label = wgpu::StringView("Uniforms Bind Group Layout");
    wgpu::BindGroupLayout transformsBindGroupLayout = device.createBindGroupLayout(transformBindGroupLayoutDesc);




	WGPUBindGroupLayoutEntry shadowDataBindingLayoutUniforms = {0};
    shadowDataBindingLayoutUniforms.binding = 0;
    shadowDataBindingLayoutUniforms.visibility = wgpu::ShaderStage::Vertex;
    shadowDataBindingLayoutUniforms.buffer.type = wgpu::BufferBindingType::Uniform;
    shadowDataBindingLayoutUniforms.buffer.minBindingSize = sizeof(glm::mat4);

    std::array<WGPUBindGroupLayoutEntry, 1> shadowBindings = { shadowDataBindingLayoutUniforms };

	wgpu::BindGroupLayoutDescriptor shadowDatabindGroupLayoutDesc(wgpu::Default);
    shadowDatabindGroupLayoutDesc.entryCount = shadowBindings.size();
    shadowDatabindGroupLayoutDesc.entries = shadowBindings.data();
    shadowDatabindGroupLayoutDesc.label = wgpu::StringView("Uniforms Bind Group Layout");
    wgpu::BindGroupLayout shadowDataBindGroupLayout = device.createBindGroupLayout(shadowDatabindGroupLayoutDesc);





    std::array<WGPUBindGroupLayout, 2> bindGroupLayouts = { transformsBindGroupLayout, shadowDataBindGroupLayout };






	wgpu::PipelineLayoutDescriptor layoutDesc(wgpu::Default);
	layoutDesc.bindGroupLayoutCount = bindGroupLayouts.size();
	layoutDesc.bindGroupLayouts = bindGroupLayouts.data();
	wgpu::PipelineLayout layout = device.createPipelineLayout(layoutDesc);

	std::array<WGPUVertexBufferLayout, 2> vertexBuffers = { vertexBufferLayout, vertexBufferLayoutTransformsIndex };

    pipelineDesc.vertex.bufferCount = static_cast<uint32_t>(vertexBuffers.size());
    pipelineDesc.vertex.buffers = vertexBuffers.data();
	pipelineDesc.vertex.module = shaderModule;
    pipelineDesc.vertex.entryPoint = wgpu::StringView("vs_main");

	wgpu::FragmentState fragmentState(wgpu::Default);
    fragmentState.module = shaderModule;
	fragmentState.entryPoint = wgpu::StringView("fs_main");

	// wgpu::ColorTargetState colorTarget(wgpu::Default);
    // colorTarget.format = surfaceFormat;
	// colorTarget.writeMask = wgpu::ColorWriteMask::All;

	wgpu::BlendState blendState(wgpu::Default);
    fragmentState.targetCount = 0;
    fragmentState.targets = nullptr;
    pipelineDesc.fragment = &fragmentState;
	pipelineDesc.layout = layout;

	auto &window = core.GetResource<ES::Plugin::Window::Resource::Window>();

	wgpu::DepthStencilState depthStencilState(wgpu::Default);
	depthStencilState.depthCompare = wgpu::CompareFunction::Less;
	depthStencilState.depthWriteEnabled = wgpu::OptionalBool::True;
	depthStencilState.format = wgpu::TextureFormat::Depth32Float;
	pipelineDesc.depthStencil = &depthStencilState;

    pipelineDesc.primitive.cullMode = wgpu::CullMode::Back;
    pipelineDesc.primitive.frontFace = wgpu::FrontFace::CCW;

	// TODO: Use async pipeline creation
	wgpu::RenderPipeline pipeline = device.createRenderPipeline(pipelineDesc);

	if (pipeline == nullptr) throw std::runtime_error("Could not create render pipeline");

	shaderModule.release();

	core.GetResource<Pipelines>().renderPipelines["ShadowPass"] = PipelineData{
		.pipeline = pipeline,
		.bindGroupLayouts = {transformsBindGroupLayout, shadowDataBindGroupLayout},
		.layout = layout,
	};
}
}
