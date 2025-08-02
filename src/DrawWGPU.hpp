#pragma once

#include "webgpu.hpp"
#include "Engine.hpp"
#include "structs.hpp"
#include "Sprite.hpp"
#include <imgui.h>
#include <backends/imgui_impl_wgpu.h>
#include <backends/imgui_impl_glfw.h>


void UpdateGui(wgpu::RenderPassEncoder renderPass, ES::Engine::Core &core) {
    // Start the Dear ImGui frame
    ImGui_ImplWGPU_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

	auto &clearColor = core.GetResource<ClearColor>();

	// Build our UI
	static float f = 0.0f;
	static int counter = 0;
	static bool show_demo_window = true;
	static bool show_another_window = false;

	ImGui::Begin("Hello, world!"); // Create a window called "Hello, world!"

	glm::vec3 color = glm::vec3(clearColor.value.r, clearColor.value.g, clearColor.value.b);
	ImGui::ColorEdit3("clear color", (float*)&color); // Edit 3 floats representing a color
	clearColor.value = { color.r, color.g, color.b, clearColor.value.a };

	ImGuiIO& io = ImGui::GetIO();
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

	core.GetRegistry().view<Mesh, Name>().each([&](Mesh &mesh, Name &name) {
		ImGui::Checkbox(name.value.c_str(), &mesh.enabled);
	});

	core.GetRegistry().view<Sprite, Name>().each([&](Sprite &sprite, Name &name) {
		ImGui::Checkbox(name.value.c_str(), &sprite.enabled);
	});

	auto &lights = core.GetResource<std::vector<Light>>();

	ImGui::Text("Lights: %zu", lights.size());
	bool lightsDirty = false;
	if (ImGui::Button("Clear Lights")) {
		lights.clear();
		lightsDirty = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Add Light")) {
		lights.push_back(Light{
			.color = { 0.0f, 0.0f, 0.0f, 1.0f },
			.direction = { 0.0f, 0.0f, 0.0f },
			.intensity = 0.f,
			.enabled = true,
			.type = Light::Type::Point
		});
		lightsDirty = true;
	}

	ImGui::BeginChild("Lights", ImVec2(0, 400));
	for (size_t i = 0; i < lights.size(); ++i) {
		ImGui::PushID(i);
		ImGui::Text("Light %zu", i);
		ImGui::SameLine();
		ImGui::Checkbox("Enabled", (bool *)&lights[i].enabled);
		ImGui::SameLine();
		if (ImGui::Button("Remove")) {
			lights.erase(lights.begin() + i);
			lightsDirty = true;
			ImGui::PopID();
			continue;
		}
		ImGui::ColorEdit4("Color", glm::value_ptr(lights[i].color));
		ImGui::DragFloat3("Direction(Directional)/Position(Point)", glm::value_ptr(lights[i].direction), 0.1f);
		ImGui::DragFloat("Intensity", &lights[i].intensity, (lights[i].type == Light::Type::Directional ? 0.1f : 10.f));
		ImGui::Combo("Type", (int *)&lights[i].type, "Directional\0Point\0Spot\0");
		ImGui::PopID();
	}
	ImGui::EndChild();

	if (lightsDirty) {
		auto &device = core.GetResource<wgpu::Device>();
		auto &pipelineData = core.GetResource<Pipelines>().renderPipelines["3D"];
		auto &bindGroups = core.GetResource<BindGroups>();
		auto &queue = core.GetResource<wgpu::Queue>();

		lightsBuffer.destroy();
		lightsBuffer.release();

		wgpu::BufferDescriptor lightsBufferDesc(wgpu::Default);
		lightsBufferDesc.size = sizeof(Light) * std::max(lights.size(), size_t(1)) + sizeof(uint32_t) + 12 /* (padding) */;
		lightsBufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage;
		lightsBufferDesc.label = wgpu::StringView("Lights Buffer");
		lightsBuffer = device.createBuffer(lightsBufferDesc);

		uint32_t lightsCount = static_cast<uint32_t>(lights.size());
		queue.writeBuffer(lightsBuffer, 0, &lightsCount, sizeof(uint32_t));
		queue.writeBuffer(lightsBuffer, sizeof(uint32_t), lights.data(), sizeof(Light) * lights.size());

		bindGroups.groups["2"].release();

		wgpu::BindGroupEntry bindingLights(wgpu::Default);
		bindingLights.binding = 0;
		bindingLights.buffer = lightsBuffer;
		bindingLights.size = sizeof(Light) * std::max(lights.size(), size_t(1)) + sizeof(uint32_t) + 12 /* (padding) */; // TODO: Resize when adding a new light

		std::array<wgpu::BindGroupEntry, 1> lightsBindings = { bindingLights };

		wgpu::BindGroupDescriptor bindGroupLightsDesc(wgpu::Default);
		bindGroupLightsDesc.layout = pipelineData.bindGroupLayouts[1];
		bindGroupLightsDesc.entryCount = lightsBindings.size();
		bindGroupLightsDesc.entries = lightsBindings.data();
		bindGroupLightsDesc.label = wgpu::StringView("Lights Bind Group");
		auto bg = device.createBindGroup(bindGroupLightsDesc);

		bindGroups.groups["2"] = bg;
	}

	ImGui::End();

    // Draw the UI
    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), renderPass);
}

void DrawMesh(ES::Engine::Core &core, Mesh &mesh, ES::Plugin::Object::Component::Transform &transform) {
	wgpu::Queue &queue = core.GetResource<wgpu::Queue>();
	PipelineData &pipelineData = core.GetResource<Pipelines>().renderPipelines["3D"];
	wgpu::Device &device = core.GetResource<wgpu::Device>();

	const auto &transformMatrix = transform.getTransformationMatrix();

	queue.writeBuffer(uniformBuffer, offsetof(MyUniforms, modelMatrix), &transformMatrix, sizeof(MyUniforms::modelMatrix));


	if (!textureView) throw std::runtime_error("Texture view is not created, cannot draw mesh.");
	wgpu::CommandEncoderDescriptor encoderDesc(wgpu::Default);
	encoderDesc.label = wgpu::StringView("My command encoder");
	auto commandEncoder = device.createCommandEncoder(encoderDesc);
	if (commandEncoder == nullptr) throw std::runtime_error("Command encoder is not created, cannot draw mesh.");

	wgpu::RenderPassDescriptor renderPassDesc(wgpu::Default);
	renderPassDesc.label = wgpu::StringView("Mesh render pass");

	wgpu::RenderPassColorAttachment renderPassColorAttachment(wgpu::Default);
	renderPassColorAttachment.view = textureView;
	renderPassColorAttachment.loadOp = wgpu::LoadOp::Load;
	renderPassColorAttachment.storeOp = wgpu::StoreOp::Store;
	renderPassColorAttachment.clearValue = wgpu::Color{ 0.0, 0.0, 0.0, 1.0 };

	renderPassDesc.colorAttachmentCount = 1;
	renderPassDesc.colorAttachments = &renderPassColorAttachment;

	wgpu::RenderPassDepthStencilAttachment depthStencilAttachment(wgpu::Default);
	depthStencilAttachment.view = depthTextureView;
	depthStencilAttachment.depthClearValue = 1.0f;
	depthStencilAttachment.depthLoadOp = wgpu::LoadOp::Load;
	depthStencilAttachment.depthStoreOp = wgpu::StoreOp::Store;

	renderPassDesc.depthStencilAttachment = &depthStencilAttachment;

	renderPass = commandEncoder.beginRenderPass(renderPassDesc);

	// Select which render pipeline to use
	renderPass.setPipeline(pipelineData.pipeline);
	auto &bindGroup1 = core.GetResource<BindGroups>().groups["1"];
	renderPass.setBindGroup(0, bindGroup1, 0, nullptr);
	auto &bindGroupLights = core.GetResource<BindGroups>().groups["2"];
	renderPass.setBindGroup(1, bindGroupLights, 0, nullptr);

	renderPass.setVertexBuffer(0, mesh.pointBuffer, 0, mesh.pointBuffer.getSize());
	renderPass.setIndexBuffer(mesh.indexBuffer, wgpu::IndexFormat::Uint32, 0, mesh.indexBuffer.getSize());

	// Set binding group
	renderPass.drawIndexed(mesh.indexCount, 1, 0, 0, 0);

	renderPass.end();
	renderPass.release();

	// Finally encode and submit the render pass
	wgpu::CommandBufferDescriptor cmdBufferDescriptor(wgpu::Default);
	cmdBufferDescriptor.label = wgpu::StringView("Command buffer");
	auto command = commandEncoder.finish(cmdBufferDescriptor);
	commandEncoder.release();

	queue.submit(1, &command);
	command.release();
}

void DrawGui(ES::Engine::Core &core)
{
	PipelineData &pipelineData = core.GetResource<Pipelines>().renderPipelines["2D"];
	wgpu::Queue &queue = core.GetResource<wgpu::Queue>();
	wgpu::Device &device = core.GetResource<wgpu::Device>();

	if (!textureView) throw std::runtime_error("Texture view is not created, cannot draw mesh.");
	wgpu::CommandEncoderDescriptor encoderDesc(wgpu::Default);
	encoderDesc.label = wgpu::StringView("My command encoder");
	auto commandEncoder = device.createCommandEncoder(encoderDesc);
	if (commandEncoder == nullptr) throw std::runtime_error("Command encoder is not created, cannot draw mesh.");

	wgpu::RenderPassDescriptor renderPassDesc(wgpu::Default);
	renderPassDesc.label = wgpu::StringView("Mesh render pass");

	wgpu::RenderPassColorAttachment renderPassColorAttachment(wgpu::Default);
	renderPassColorAttachment.view = textureView;
	renderPassColorAttachment.loadOp = wgpu::LoadOp::Load;
	renderPassColorAttachment.storeOp = wgpu::StoreOp::Store;
	renderPassColorAttachment.clearValue = wgpu::Color{ 0.0, 0.0, 0.0, 1.0 };

	renderPassDesc.colorAttachmentCount = 1;
	renderPassDesc.colorAttachments = &renderPassColorAttachment;

	wgpu::RenderPassDepthStencilAttachment depthStencilAttachment(wgpu::Default);
	depthStencilAttachment.view = depthTextureView;
	depthStencilAttachment.depthClearValue = 1.0f;
	depthStencilAttachment.depthLoadOp = wgpu::LoadOp::Load;
	depthStencilAttachment.depthStoreOp = wgpu::StoreOp::Store;

	renderPassDesc.depthStencilAttachment = &depthStencilAttachment;

	renderPass = commandEncoder.beginRenderPass(renderPassDesc);

	// Select which render pipeline to use
	renderPass.setPipeline(pipelineData.pipeline);
	auto &bindGroup = core.GetResource<BindGroups>().groups["1"];
	renderPass.setBindGroup(0, bindGroup, 0, nullptr);

	UpdateGui(renderPass, core);

	renderPass.end();
	renderPass.release();

	// Finally encode and submit the render pass
	wgpu::CommandBufferDescriptor cmdBufferDescriptor(wgpu::Default);
	cmdBufferDescriptor.label = wgpu::StringView("Command buffer");
	auto command = commandEncoder.finish(cmdBufferDescriptor);
	commandEncoder.release();

	queue.submit(1, &command);
	command.release();
}

void DrawSprite(ES::Engine::Core &core, Sprite &sprite)
{
	wgpu::Queue &queue = core.GetResource<wgpu::Queue>();
	PipelineData &pipelineData = core.GetResource<Pipelines>().renderPipelines["2D"];
	wgpu::Device &device = core.GetResource<wgpu::Device>();

	if (!textureView) throw std::runtime_error("Texture view is not created, cannot draw sprite.");
	wgpu::CommandEncoderDescriptor encoderDesc(wgpu::Default);
	encoderDesc.label = wgpu::StringView("My command encoder");
	auto commandEncoder = device.createCommandEncoder(encoderDesc);
	if (commandEncoder == nullptr) throw std::runtime_error("Command encoder is not created, cannot draw sprite.");

	wgpu::RenderPassDescriptor renderPassDesc(wgpu::Default);
	renderPassDesc.label = wgpu::StringView("Sprite render pass");

	wgpu::RenderPassColorAttachment renderPassColorAttachment(wgpu::Default);
	renderPassColorAttachment.view = textureView;
	renderPassColorAttachment.loadOp = wgpu::LoadOp::Load;
	renderPassColorAttachment.storeOp = wgpu::StoreOp::Store;
	renderPassColorAttachment.clearValue = wgpu::Color{ 0.0, 0.0, 0.0, 1.0 };

	renderPassDesc.colorAttachmentCount = 1;
	renderPassDesc.colorAttachments = &renderPassColorAttachment;

	wgpu::RenderPassDepthStencilAttachment depthStencilAttachment(wgpu::Default);
	depthStencilAttachment.view = depthTextureView;
	depthStencilAttachment.depthClearValue = 1.0f;
	depthStencilAttachment.depthLoadOp = wgpu::LoadOp::Load;
	depthStencilAttachment.depthStoreOp = wgpu::StoreOp::Store;

	renderPassDesc.depthStencilAttachment = &depthStencilAttachment;

	renderPass = commandEncoder.beginRenderPass(renderPassDesc);

	// Select which render pipeline to use
	renderPass.setPipeline(pipelineData.pipeline);

	auto &bindGroup = core.GetResource<BindGroups>().groups["2D"];
	renderPass.setBindGroup(0, bindGroup, 0, nullptr);

	auto &textures = core.GetResource<TextureManager>();
	auto texture = textures.Get(sprite.textureHandleID);
	renderPass.setBindGroup(1, texture.bindGroup, 0, nullptr);

	renderPass.setVertexBuffer(0, sprite.pointBuffer, 0, sprite.pointBuffer.getSize());
	renderPass.setIndexBuffer(sprite.indexBuffer, wgpu::IndexFormat::Uint32, 0, sprite.indexBuffer.getSize());

	renderPass.drawIndexed(sprite.indexCount, 1, 0, 0, 0);

	renderPass.end();
	renderPass.release();

	// Finally encode and submit the render pass
	wgpu::CommandBufferDescriptor cmdBufferDescriptor(wgpu::Default);
	cmdBufferDescriptor.label = wgpu::StringView("Command buffer");
	auto command = commandEncoder.finish(cmdBufferDescriptor);
	commandEncoder.release();

	queue.submit(1, &command);
	command.release();
}

void DrawMeshes(ES::Engine::Core &core)
{
	wgpu::Device &device = core.GetResource<wgpu::Device>();
	wgpu::Surface &surface = core.GetResource<wgpu::Surface>();
	wgpu::Queue &queue = core.GetResource<wgpu::Queue>();
	auto &window = core.GetResource<ES::Plugin::Window::Resource::Window>();

	MyUniforms uniforms;

	uniforms.time = glfwGetTime();
	uniforms.color = { 1.f, 1.0f, 1.0f, 1.0f };

	CameraData cameraData = core.GetResource<CameraData>();
	uniforms.viewMatrix = glm::lookAt(
		cameraData.position,
		cameraData.position + glm::vec3(
			glm::cos(cameraData.yaw) * glm::cos(cameraData.pitch),
			glm::sin(cameraData.pitch),
			glm::sin(cameraData.yaw) * glm::cos(cameraData.pitch)
		),
		cameraData.up
	);

	uniforms.projectionMatrix = glm::perspective(
		cameraData.fovY,
		cameraData.aspectRatio,
		cameraData.nearPlane,
		cameraData.farPlane
	);

	uniforms.cameraPosition = cameraData.position;

	queue.writeBuffer(uniformBuffer, offsetof(MyUniforms, time), &uniforms.time, sizeof(MyUniforms::time));
	queue.writeBuffer(uniformBuffer, offsetof(MyUniforms, color), &uniforms.color, sizeof(MyUniforms::color));
	queue.writeBuffer(uniformBuffer, offsetof(MyUniforms, viewMatrix), &uniforms.viewMatrix, sizeof(MyUniforms::viewMatrix));
	queue.writeBuffer(uniformBuffer, offsetof(MyUniforms, projectionMatrix), &uniforms.projectionMatrix, sizeof(MyUniforms::projectionMatrix));
	queue.writeBuffer(uniformBuffer, offsetof(MyUniforms, cameraPosition), &uniforms.cameraPosition, sizeof(MyUniforms::cameraPosition));

	auto &lights = core.GetResource<std::vector<Light>>();
	uint32_t lightsCount = static_cast<uint32_t>(lights.size());

	queue.writeBuffer(lightsBuffer, 0, &lightsCount, sizeof(uint32_t));
	queue.writeBuffer(lightsBuffer, sizeof(uint32_t) + 12 /* (padding) */, lights.data(), sizeof(Light) * lights.size());

	Uniforms2D uniforms2D;

	glm::ivec2 windowSize = window.GetSize();

	uniforms2D.orthoMatrix = glm::ortho(
		windowSize.x * -0.5f,
		windowSize.x * 0.5f,
		windowSize.y * -0.5f,
		windowSize.y * 0.5f);

	queue.writeBuffer(uniform2DBuffer, 0, &uniforms2D, sizeof(Uniforms2D));

	core.GetRegistry().view<Mesh, ES::Plugin::Object::Component::Transform>().each([&](Mesh &mesh, ES::Plugin::Object::Component::Transform &transform) {
		if (!mesh.enabled) return;
		DrawMesh(core, mesh, transform);
	});

	core.GetRegistry().view<Sprite>().each([&](Sprite &sprite) {
		if (!sprite.enabled) return;
		DrawSprite(core, sprite);
	});

	DrawGui(core);

	// At the end of the frame
	textureView.release();
	surface.present();
}
