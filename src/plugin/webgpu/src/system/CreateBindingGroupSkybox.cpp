#include "CreateBindingGroupSkybox.hpp"
#include "structs.hpp"

static void SetupBindingGroupSkybox(ES::Engine::Core &core)
{
	auto &device = core.GetResource<wgpu::Device>();
	auto &pipelineData = core.GetResource<Pipelines>().renderPipelines["Skybox"];
	auto &bindGroups = core.GetResource<BindGroups>();
	auto &textureManager = core.GetResource<TextureManager>();
	const std::string name = "Skybox";
	//TODO: Should we separate this from pipelineData?

	if (device == nullptr) throw std::runtime_error("WebGPU device is not created, cannot create binding group.");

	wgpu::BindGroupEntry transformBinding(wgpu::Default);
	transformBinding.binding = 0;
	transformBinding.buffer = skyboxBuffer;
	transformBinding.size = sizeof(glm::mat4);

	wgpu::BindGroupEntry skyboxTextureBinding(wgpu::Default);
	skyboxTextureBinding.binding = 1;
	skyboxTextureBinding.textureView = textureManager.Get("SkyboxTexture").textureView;

	wgpu::BindGroupEntry skyboxSamplerBinding(wgpu::Default);
	skyboxSamplerBinding.binding = 2;
	skyboxSamplerBinding.sampler = textureManager.Get("SkyboxTexture").sampler;

	std::array<wgpu::BindGroupEntry, 3> bindings = { transformBinding, skyboxTextureBinding, skyboxSamplerBinding };

	wgpu::BindGroupDescriptor bindGroupDesc(wgpu::Default);
	bindGroupDesc.layout = pipelineData.bindGroupLayouts[0];
	bindGroupDesc.entryCount = bindings.size();
	bindGroupDesc.entries = bindings.data();
	bindGroupDesc.label = wgpu::StringView(fmt::format("{} Bind Group", name));
	auto bg1 = device.createBindGroup(bindGroupDesc);

	if (bg1 == nullptr) throw std::runtime_error("Could not create WebGPU bind group");

	bindGroups.groups[name] = bg1;
}

namespace ES::Plugin::WebGPU::System {
void CreateBindingGroupSkybox(ES::Engine::Core &core)
{
	SetupBindingGroupSkybox(core);

	core.GetResource<WindowResizeCallbacks>().callbacks.push_back([](ES::Engine::Core &core, int width, int height) {
		auto &bindGroups = core.GetResource<BindGroups>();
		bindGroups.groups["Skybox"].release();
		bindGroups.groups.erase("Skybox");

		SetupBindingGroupSkybox(core);
	});
}
}
