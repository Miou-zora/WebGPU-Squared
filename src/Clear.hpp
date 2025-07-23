#pragma once

#include "webgpu.hpp"
#include "Engine.hpp"

void Clear(ES::Engine::Core &core) {
	wgpu::Device &device = core.GetResource<wgpu::Device>();
	wgpu::Surface &surface = core.GetResource<wgpu::Surface>();
	wgpu::Queue &queue = core.GetResource<wgpu::Queue>();
	const ClearColor &clearColor = core.GetResource<ClearColor>();

	if (device == nullptr) throw std::runtime_error("WebGPU device is not created, cannot draw.");
	if (surface == nullptr) throw std::runtime_error("WebGPU surface is not created, cannot draw.");

	textureView = GetNextSurfaceViewData(surface);
	if (textureView == nullptr) throw std::runtime_error("Could not get next surface texture view");

	wgpu::CommandEncoderDescriptor encoderDesc(wgpu::Default);
	encoderDesc.label = wgpu::StringView("Clear command encoder");
	auto encoder = device.createCommandEncoder(encoderDesc);

	wgpu::RenderPassDescriptor renderPassDesc(wgpu::Default);
	renderPassDesc.label = wgpu::StringView("Clear render pass");

	wgpu::RenderPassColorAttachment renderPassColorAttachment(wgpu::Default);
	renderPassColorAttachment.view = textureView;
	renderPassColorAttachment.loadOp = wgpu::LoadOp::Clear;
	renderPassColorAttachment.storeOp = wgpu::StoreOp::Store;
	renderPassColorAttachment.clearValue = clearColor.value;

	renderPassDesc.colorAttachmentCount = 1;
	renderPassDesc.colorAttachments = &renderPassColorAttachment;

	wgpu::RenderPassDepthStencilAttachment depthStencilAttachment(wgpu::Default);
	depthStencilAttachment.view = depthTextureView;
	depthStencilAttachment.depthLoadOp = wgpu::LoadOp::Clear;
	depthStencilAttachment.depthStoreOp = wgpu::StoreOp::Store;
	depthStencilAttachment.depthClearValue = 1.0f;

	renderPassDesc.depthStencilAttachment = &depthStencilAttachment;

	renderPass = encoder.beginRenderPass(renderPassDesc);

	if (renderPass == nullptr) throw std::runtime_error("Could not create render pass encoder");

	renderPass.end();
	renderPass.release();
	wgpu::CommandBufferDescriptor cmdBufferDescriptor(wgpu::Default);
	cmdBufferDescriptor.label = wgpu::StringView("Clear command buffer");
	auto command = encoder.finish(cmdBufferDescriptor);
	encoder.release();
	queue.submit(1, &command);
	command.release();
}
