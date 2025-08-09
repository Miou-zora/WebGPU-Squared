#pragma once

#include "webgpu.hpp"
#include "Engine.hpp"
#include "structs.hpp"
#include "Sprite.hpp"
#include <imgui.h>
#include <backends/imgui_impl_wgpu.h>
#include <backends/imgui_impl_glfw.h>
#include "UpdateLights.hpp"


void RenderGui(wgpu::RenderPassEncoder renderPass, ES::Engine::Core &core) {
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
		UpdateLights(core);
	}

	ImGui::End();

    // Draw the UI
    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), renderPass);
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
}
