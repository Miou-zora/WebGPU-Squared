
#include "ConfigureSurface.hpp"
#include "WebGPU.hpp"
#include "resource/window/Window.hpp"
#include "component/Transform.hpp"

void ES::Plugin::WebGPU::System::UpdateBufferUniforms(ES::Engine::Core &core) {
	auto &device = core.GetResource<wgpu::Device>();
    auto &pipelineData = core.GetResource<Pipelines>().renderPipelines["GBuffer"];
    auto &bindGroups = core.GetResource<BindGroups>();
    auto &queue = core.GetResource<wgpu::Queue>();
	size_t entityCount = 0;

	core.GetRegistry().view<ES::Plugin::WebGPU::Component::Mesh, ES::Plugin::Object::Component::Transform>().each([&](auto entity, ES::Plugin::WebGPU::Component::Mesh &mesh, ES::Plugin::Object::Component::Transform &transform) {
		if (mesh.pipelineType != PipelineType::_3D)
			return;
		entityCount++;
	});

	size_t offset = 0;

	if (uniformsBuffer.getSize() == sizeof(Uniforms) * entityCount) {
		core.GetRegistry().view<ES::Plugin::WebGPU::Component::Mesh, ES::Plugin::Object::Component::Transform>().each([&](auto entity, ES::Plugin::WebGPU::Component::Mesh &mesh, ES::Plugin::Object::Component::Transform &transform) {
			if (mesh.pipelineType != PipelineType::_3D)
				return;
			Uniforms uniforms;
			uniforms.modelMatrix = transform.getTransformationMatrix();
			uniforms.normalModelMatrix = glm::transpose(glm::inverse(uniforms.modelMatrix));
			queue.writeBuffer(uniformsBuffer, offset, &uniforms, sizeof(uniforms));
			offset += sizeof(uniforms);
		});
		return;
	}

	wgpu::BufferDescriptor bufferDesc(wgpu::Default);
	bufferDesc.size = sizeof(Uniforms) * std::max(entityCount, size_t(1));
    bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage;
    bufferDesc.label = wgpu::StringView("Uniforms Buffer for GBuffer");
    uniformsBuffer = device.createBuffer(bufferDesc);

	core.GetRegistry().view<ES::Plugin::WebGPU::Component::Mesh, ES::Plugin::Object::Component::Transform>().each([&](auto entity, ES::Plugin::WebGPU::Component::Mesh &mesh, ES::Plugin::Object::Component::Transform &transform) {
		if (mesh.pipelineType != PipelineType::_3D)
			return;
		Uniforms uniforms;
		uniforms.modelMatrix = transform.getTransformationMatrix();
		uniforms.normalModelMatrix = glm::transpose(glm::inverse(uniforms.modelMatrix));
		queue.writeBuffer(uniformsBuffer, offset, &uniforms, sizeof(uniforms));
		offset += sizeof(uniforms);
	});

	wgpu::BindGroupEntry bindingUniforms(wgpu::Default);
    bindingUniforms.binding = 0;
    bindingUniforms.buffer = uniformsBuffer;
    bindingUniforms.size = sizeof(Uniforms) * entityCount;

    std::array<wgpu::BindGroupEntry, 1> bindingsUniforms = { bindingUniforms };
	wgpu::BindGroupDescriptor bindGroupDesc(wgpu::Default);
	bindGroupDesc.layout = pipelineData.bindGroupLayouts[2];
    bindGroupDesc.entryCount = bindingsUniforms.size();
    bindGroupDesc.entries = bindingsUniforms.data();
    bindGroupDesc.label = wgpu::StringView("GBuffer Binding Group Uniforms");
    auto bg2 = device.createBindGroup(bindGroupDesc);
    if (bg2 == nullptr) throw std::runtime_error("Could not create WebGPU bind group");
    bindGroups.groups["GBufferUniforms"] = bg2;
}
