#include "InitializeSkyboxPipeline.hpp"
#include "WebGPU.hpp"
#include "resource/Window/Window.hpp"
#include "utils.hpp"
#include "structs.hpp"

namespace ES::Plugin::WebGPU::System {

void InitializeSkyboxPipeline(ES::Engine::Core &core)
{
	auto &device = core.GetResource<wgpu::Device>();
	auto &surface = core.GetResource<wgpu::Surface>();
	auto &window = core.GetResource<ES::Plugin::Window::Resource::Window>();
	wgpu::TextureFormat surfaceFormat = core.GetResource<wgpu::SurfaceCapabilities>().formats[0];
	const std::string pipelineName = "Skybox";
	const std::string shaderfilePath = "./assets/shader/shaderSkybox.wgsl";

	if (device == nullptr) throw std::runtime_error(fmt::format("WebGPU device is not created, cannot initialize {} pipeline.", pipelineName));
	if (surface == nullptr) throw std::runtime_error(fmt::format("WebGPU surface is not created, cannot initialize {} pipeline.", pipelineName));

	wgpu::RenderPipelineDescriptor pipelineDesc(wgpu::Default);
	pipelineDesc.label = wgpu::StringView(fmt::format("{} Render Pipeline", pipelineName));

	wgpu::ShaderSourceWGSL wgslDesc(wgpu::Default);
	std::string wgslSource = loadFile(shaderfilePath);
	wgslDesc.code = wgpu::StringView(wgslSource);
	wgpu::ShaderModuleDescriptor shaderDesc(wgpu::Default);
    shaderDesc.nextInChain = &wgslDesc.chain; // connect the chained extension
    shaderDesc.label = wgpu::StringView(fmt::format("{} Shader Module", pipelineName));
	wgpu::ShaderModule shaderModule = device.createShaderModule(shaderDesc);
	wgpu::VertexBufferLayout vertexBufferLayout(wgpu::Default);

	std::vector<wgpu::VertexAttribute> vertexAttribs(2);
    vertexAttribs[0].shaderLocation = 0;
    vertexAttribs[0].format = wgpu::VertexFormat::Float32x3;
    vertexAttribs[0].offset = 0;
	vertexAttribs[1].shaderLocation = 1;
	vertexAttribs[1].format = wgpu::VertexFormat::Float32x2;
	vertexAttribs[1].offset = 3 * sizeof(float);
    vertexBufferLayout.attributeCount = static_cast<uint32_t>(vertexAttribs.size());
    vertexBufferLayout.attributes = vertexAttribs.data();
    vertexBufferLayout.arrayStride = 5 * sizeof(float);
    vertexBufferLayout.stepMode = wgpu::VertexStepMode::Vertex;

	// TODO: find why it does not work with wgpu::BindGroupLayoutEntry
	WGPUBindGroupLayoutEntry uniformBindingLayout = {0};
	uniformBindingLayout.binding = 0;
	uniformBindingLayout.visibility = wgpu::ShaderStage::Vertex;
	uniformBindingLayout.buffer.type = wgpu::BufferBindingType::Uniform;
	uniformBindingLayout.buffer.minBindingSize = sizeof(glm::mat4);

	WGPUBindGroupLayoutEntry textureBindingLayout = {0};
	textureBindingLayout.binding = 1;
	textureBindingLayout.visibility = wgpu::ShaderStage::Fragment;
	textureBindingLayout.texture.sampleType = wgpu::TextureSampleType::Float;
	textureBindingLayout.texture.viewDimension = wgpu::TextureViewDimension::Cube;

	WGPUBindGroupLayoutEntry samplerBindingLayout = {0};
	samplerBindingLayout.binding = 2;
	samplerBindingLayout.visibility = wgpu::ShaderStage::Fragment;
	samplerBindingLayout.sampler.type = wgpu::SamplerBindingType::Filtering;

	std::array<WGPUBindGroupLayoutEntry, 3> uniformsBindings = { uniformBindingLayout, textureBindingLayout, samplerBindingLayout };

	wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc(wgpu::Default);
	bindGroupLayoutDesc.entryCount = uniformsBindings.size();
	bindGroupLayoutDesc.entries = uniformsBindings.data();
	bindGroupLayoutDesc.label = wgpu::StringView("Uniforms Bind Group Layout");
	wgpu::BindGroupLayout uniformsBindGroupLayout = device.createBindGroupLayout(bindGroupLayoutDesc);

	std::array<WGPUBindGroupLayout, 1> bindGroupLayouts = { uniformsBindGroupLayout };

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

	wgpu::BlendState blendState(wgpu::Default);

	wgpu::ColorTargetState colorTarget(wgpu::Default);
    colorTarget.format = wgpu::TextureFormat::RGBA16Float;
	colorTarget.writeMask = wgpu::ColorWriteMask::All;
    colorTarget.blend = &blendState;
    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTarget;
    pipelineDesc.fragment = &fragmentState;
	pipelineDesc.layout = layout;

	// No depth buffer
	pipelineDesc.depthStencil = nullptr;

	wgpu::RenderPipeline pipeline = device.createRenderPipeline(pipelineDesc);

	if (pipeline == nullptr) throw std::runtime_error("Could not create render pipeline");

	wgpuShaderModuleRelease(shaderModule);

	core.GetResource<Pipelines>().renderPipelines["Skybox"] = PipelineData{
		.pipeline = pipeline,
		.bindGroupLayouts = {
			uniformsBindGroupLayout
		},
		.layout = layout,
	};
}

}
