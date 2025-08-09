#include "RenderGUI.hpp"
#include "structs.hpp"
#include "Sprite.hpp"
#include <imgui.h>
#include <backends/imgui_impl_wgpu.h>
#include <backends/imgui_impl_glfw.h>
#include "UpdateLights.hpp"
#include <glm/gtc/type_ptr.hpp>

namespace ES::Plugin::ImGUI::WebGPU::Util {

void RenderGUI(wgpu::RenderPassEncoder renderPass, ES::Engine::Core &core) {
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

	core.GetRegistry().view<ES::Plugin::WebGPU::Component::Mesh, Name>().each([&](ES::Plugin::WebGPU::Component::Mesh &mesh, Name &name) {
		ImGui::Checkbox(name.value.c_str(), &mesh.enabled);
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
		ES::Plugin::WebGPU::Utils::UpdateLights(core);
	}

	ImGui::End();

    // Draw the UI
    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), renderPass);
}
}
