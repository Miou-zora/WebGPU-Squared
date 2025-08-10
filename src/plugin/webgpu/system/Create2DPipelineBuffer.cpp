#include "Create2DPipelineBuffer.hpp"
#include "WebGPU.hpp"

namespace ES::Plugin::WebGPU::System {

void Create2DPipelineBuffer(ES::Engine::Core &core)
{
	wgpu::Queue &queue = core.GetResource<wgpu::Queue>();
	wgpu::Device &device = core.GetResource<wgpu::Device>();

	if (queue == nullptr) throw std::runtime_error("WebGPU queue is not created, cannot initialize buffers.");
	if (device == nullptr) throw std::runtime_error("WebGPU device is not created, cannot initialize buffers.");

	wgpu::BufferDescriptor bufferDesc(wgpu::Default);
	bufferDesc.size = sizeof(Uniforms2D);
	bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
	uniform2DBuffer = device.createBuffer(bufferDesc);

	Uniforms2D uniforms;
	uniforms.orthoMatrix = glm::ortho(
		-400.0f, 400.0f,
		-400.0f, 400.0f
	);
	queue.writeBuffer(uniform2DBuffer, 0, &uniforms, sizeof(uniforms));
}
}
