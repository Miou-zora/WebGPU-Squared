#include "CreateBindingGroupDeferred.hpp"
#include "structs.hpp"

static void SetupBindingGroupDeferred(ES::Engine::Core &core)
{
	auto &device = core.GetResource<wgpu::Device>();
	auto &pipelineData = core.GetResource<Pipelines>().renderPipelines["Deferred"];
	auto &bindGroups = core.GetResource<BindGroups>();
	auto &textureManager = core.GetResource<TextureManager>();
	//TODO: Put this in a separate system
	//TODO: Should we separate this from pipelineData?

	if (device == nullptr) throw std::runtime_error("WebGPU device is not created, cannot create binding group.");

	wgpu::BindGroupEntry bindingNormal(wgpu::Default);
	bindingNormal.binding = 0;
	bindingNormal.textureView = textureManager.Get("gBufferTexture2DFloat16").textureView;

	wgpu::BindGroupEntry bindingAlbedo(wgpu::Default);
	bindingAlbedo.binding = 1;
	bindingAlbedo.textureView = textureManager.Get("gBufferTextureAlbedo").textureView;

	wgpu::BindGroupEntry bindingDepth(wgpu::Default);
	bindingDepth.binding = 2;
	bindingDepth.textureView = textureManager.Get("depthTexture").textureView;

	std::array<wgpu::BindGroupEntry, 3> bindings = { bindingNormal, bindingAlbedo, bindingDepth };

	wgpu::BindGroupDescriptor bindGroupDesc(wgpu::Default);
	bindGroupDesc.layout = pipelineData.bindGroupLayouts[0];
	bindGroupDesc.entryCount = bindings.size();
	bindGroupDesc.entries = bindings.data();
	bindGroupDesc.label = wgpu::StringView("Deferred Binding Group Textures");
	auto bg0 = device.createBindGroup(bindGroupDesc);

	if (bg0 == nullptr) throw std::runtime_error("Could not create WebGPU bind group");

	bindGroups.groups["DeferredGroup0"] = bg0;

	wgpu::BindGroupEntry bindingCamera(wgpu::Default);
	bindingCamera.binding = 0;
	bindingCamera.buffer = cameraBuffer;
	bindingCamera.size = sizeof(glm::mat4) * 2 + sizeof(glm::vec3) + sizeof(float);

	std::array<wgpu::BindGroupEntry, 1> bindingsCamera = { bindingCamera };

	bindGroupDesc.layout = pipelineData.bindGroupLayouts[2];
	bindGroupDesc.entryCount = bindingsCamera.size();
	bindGroupDesc.entries = bindingsCamera.data();
	bindGroupDesc.label = wgpu::StringView("Deferred Binding Group Camera");
	auto bg2 = device.createBindGroup(bindGroupDesc);

	bindGroups.groups["DeferredGroup2"] = bg2;
}

namespace ES::Plugin::WebGPU::System {
void CreateBindingGroupDeferred(ES::Engine::Core &core)
{
	SetupBindingGroupDeferred(core);

	core.GetResource<WindowResizeCallbacks>().callbacks.push_back([](ES::Engine::Core &core, int width, int height) {
		auto &bindGroups = core.GetResource<BindGroups>();
		bindGroups.groups["DeferredGroup0"].release();
		bindGroups.groups["DeferredGroup2"].release();
		bindGroups.groups.erase("DeferredGroup0");
		bindGroups.groups.erase("DeferredGroup2");

		SetupBindingGroupDeferred(core);
	});
}
}
