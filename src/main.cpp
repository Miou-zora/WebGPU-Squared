#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#include <glm/ext.hpp>

#include "WebGPU.hpp"
#include <iostream>
#include <vector>
#include <fstream>
#include "Engine.hpp"
#include "PluginWindow.hpp"
#include "Input.hpp"
#include "Window.hpp"
#include "Object.hpp"
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GLFW/glfw3.h>
#include "RenderingPipeline.hpp"

#include <imgui.h>
#include <backends/imgui_impl_wgpu.h>
#include <backends/imgui_impl_glfw.h>

#include "RenderGUI.hpp"

// TODO: check learn webgpu c++ why I had this variable
uint32_t uniformStride = 0;

float cameraScale = 100.0f;

struct DragState {
    bool active = false;
    glm::vec2 startMouse = { 0.0f, 0.0f };
    float originYaw = 0.0f;
    float originPitch = 0.0f;

    // Constant settings
    float sensitivity = 0.005f;
    float scrollSensitivity = 0.1f;
	glm::vec2 velocity = {0.0, 0.0};
    glm::vec2 previousDelta = {0.0, 0.0};
    float inertia = 0.9f;
};

namespace ES::Plugin::ImGUI::WebGPU {
class Plugin : public ES::Engine::APlugin {
  public:
    using APlugin::APlugin;
    ~Plugin() = default;

    void Bind() final {
		RequirePlugins<ES::Plugin::WebGPU::Plugin>();

		RegisterSystems<ES::Plugin::RenderingPipeline::Setup>(
			[](ES::Engine::Core &core) {
				auto &window = core.GetResource<ES::Plugin::Window::Resource::Window>();
				IMGUI_CHECKVERSION();
				ImGui::CreateContext();
				ImGuiIO& io = ImGui::GetIO();

				io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
				io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
				io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
				io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

				ImGui_ImplGlfw_InitForOther(window.GetGLFWWindow(), true);
			},
			[](ES::Engine::Core &core) {
				ImGui_ImplWGPU_InitInfo info = ImGui_ImplWGPU_InitInfo();
				info.DepthStencilFormat = depthTextureFormat;
				info.RenderTargetFormat = core.GetResource<wgpu::SurfaceCapabilities>().formats[0];
				info.Device = core.GetResource<wgpu::Device>();
				ImGui_ImplWGPU_Init(&info);
			}
		);

		RegisterSystems<ES::Plugin::RenderingPipeline::ToGPU>(
			[](ES::Engine::Core &core) {
				ES::Plugin::WebGPU::Util::CustomRenderPass(core,
					RenderPassData{
						.name = "GUIRenderPass",
						.outputColorTextureName = "WindowColorTexture",
						.outputDepthTextureName = "WindowDepthTexture",
						.loadOp = wgpu::LoadOp::Load,
						.uniqueRenderCallback = Util::RenderGUI
					}
				);
			}
		);
	}
};
}

static glm::vec3 GetKeyboardMovementForce(ES::Engine::Core &core)
{
    glm::vec3 force(0.0f, 0.0f, 0.0f);
	auto &inputManager = core.GetResource<ES::Plugin::Input::Resource::InputManager>();

    if (inputManager.IsKeyPressed(GLFW_KEY_W)) {
        force.z += 1.0f;
    }
    if (inputManager.IsKeyPressed(GLFW_KEY_S)) {
        force.z -= 1.0f;
    }
    if (inputManager.IsKeyPressed(GLFW_KEY_A)) {
        force.x -= 1.0f;
    }
    if (inputManager.IsKeyPressed(GLFW_KEY_D)) {
        force.x += 1.0f;
    }
	if (inputManager.IsKeyPressed(GLFW_KEY_SPACE)) {
		force.y += 1.0f;
	}
	if (inputManager.IsKeyPressed(GLFW_KEY_LEFT_SHIFT)) {
		force.y -= 1.0f;
	}

    if (glm::length(force) > 1.0f) {
        force = glm::normalize(force);
    }

    return force;
}

static void MovementSystem(ES::Engine::Core &core)
{
	auto &cameraData = core.GetResource<CameraData>();

	glm::vec3 forwardDir = glm::vec3(
		glm::cos(cameraData.yaw) * glm::cos(cameraData.pitch),
		glm::sin(cameraData.pitch),
		glm::sin(cameraData.yaw) * glm::cos(cameraData.pitch)
	);
	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 rightDir = glm::normalize(glm::cross(forwardDir, up));
	glm::vec3 downDir = glm::normalize(glm::cross(rightDir, forwardDir));
	glm::vec3 movementForce = GetKeyboardMovementForce(core);
	if (glm::length(movementForce) > 0.0f) {
		glm::vec3 movementDirection =
			forwardDir * movementForce.z +
			downDir * movementForce.y +
			rightDir * movementForce.x;
		cameraData.position += movementDirection * 1.f * cameraScale * core.GetScheduler<ES::Engine::Scheduler::Update>().GetDeltaTime();
	}

}

class CameraPlugin : public ES::Engine::APlugin {
	public:
		using APlugin::APlugin;
    	~CameraPlugin() = default;

    	void Bind() final {
			RequirePlugins<ES::Plugin::WebGPU::Plugin, ES::Plugin::Input::Plugin>();

			RegisterResource(DragState());

			RegisterSystems<ES::Engine::Scheduler::Update>(
				MovementSystem,
				[](ES::Engine::Core &core) {
					auto &cameraData = core.GetResource<CameraData>();
					cameraData.farPlane = 100.0f * cameraScale;
					cameraData.nearPlane = 0.1f * cameraScale;
				});

			RegisterSystems<ES::Engine::Scheduler::FixedTimeUpdate>(
				[](ES::Engine::Core &core) {
					auto &drag = core.GetResource<DragState>();
					auto &cameraState = core.GetResource<CameraData>();

					constexpr float eps = 1e-4f;
					// Apply inertia only when the user released the click.
					if (!drag.active) {
						// Avoid updating the matrix when the velocity is no longer noticeable
						if (std::abs(drag.velocity.x) < eps && std::abs(drag.velocity.y) < eps) {
							return;
						}
						cameraState.pitch += drag.velocity.y * drag.sensitivity;
						cameraState.yaw += drag.velocity.x * drag.sensitivity;
						cameraState.pitch = glm::clamp(cameraState.pitch, -glm::half_pi<float>() + 1e-5f, glm::half_pi<float>() - 1e-5f);
						// Dampen the velocity so that it decreases exponentially and stops
						// after a few frames.
						drag.velocity *= drag.inertia;
					}
				}
			);

			RegisterSystems<ES::Plugin::RenderingPipeline::Setup>(
				[](ES::Engine::Core &core) {
					auto &window = core.GetResource<ES::Plugin::Window::Resource::Window>();
					auto &inputManager = core.GetResource<ES::Plugin::Input::Resource::InputManager>();

					inputManager.RegisterScrollCallback([](ES::Engine::Core &core, double, double y) {
						auto &cameraData = core.GetResource<CameraData>();
						static float sensitivity = 0.01f;
						cameraData.fovY += sensitivity * static_cast<float>(-y);
						cameraData.fovY = glm::clamp(cameraData.fovY, glm::radians(0.1f), glm::radians(179.9f));
					});

					inputManager.RegisterKeyCallback([](ES::Engine::Core &cbCore, int key, int scancode, int action, int mods) {
						// TODO: find a way to properly lock callbacks to ImGui
						ImGuiIO& io = ImGui::GetIO();
						if (io.WantCaptureKeyboard) {
							ImGui_ImplGlfw_KeyCallback(cbCore.GetResource<ES::Plugin::Window::Resource::Window>().GetGLFWWindow(), key, scancode, action, mods);
							return;
						}

						if (key == GLFW_KEY_M && action == GLFW_PRESS) {
							cameraScale *= 10.f;
						} else if (key == GLFW_KEY_N && action == GLFW_PRESS) {
							cameraScale /= 10.f;
						} else if (key == GLFW_KEY_R && action == GLFW_PRESS) {
							cameraScale = 1.0f;
						}
					});

					inputManager.RegisterMouseButtonCallback([&](ES::Engine::Core &cbCore, int button, int action, int) {
						// TODO: find a way to properly lock callbacks to ImGui
						ImGuiIO& io = ImGui::GetIO();
						if (io.WantCaptureMouse) {
							ImGui_ImplGlfw_MouseButtonCallback(cbCore.GetResource<ES::Plugin::Window::Resource::Window>().GetGLFWWindow(), button, action, 0);
						}
						auto &cameraData = cbCore.GetResource<CameraData>();
						auto &drag = cbCore.GetResource<DragState>();
						auto &window = cbCore.GetResource<ES::Plugin::Window::Resource::Window>();
						glm::vec2 mousePos = window.GetMousePosition();
						if (button == GLFW_MOUSE_BUTTON_LEFT) {
							switch(action) {
							case GLFW_PRESS:
								if (io.WantCaptureMouse) return;
								drag.active = true;
								drag.startMouse = glm::vec2(mousePos.x, -window.GetSize().y+mousePos.y);
								drag.originYaw = cameraData.yaw;
								drag.originPitch = cameraData.pitch;
								break;
							case GLFW_RELEASE:
								drag.active = false;
								break;
							}
						}
					});

					inputManager.RegisterCursorPosCallback([&](ES::Engine::Core &, double x, double y) {
						auto &cameraData = core.GetResource<CameraData>();
						auto &drag = core.GetResource<DragState>();

						if (drag.active) {
							glm::vec2 currentMouse = glm::vec2((float)x, -(float)y);
							glm::vec2 delta = (currentMouse - drag.startMouse) * drag.sensitivity;
							cameraData.yaw = drag.originYaw + delta.x;
							cameraData.pitch = drag.originPitch + delta.y;
							cameraData.pitch = glm::clamp(cameraData.pitch, -glm::half_pi<float>() + 1e-5f, glm::half_pi<float>() - 1e-5f);
							drag.velocity = delta - drag.previousDelta;
							drag.previousDelta = delta;
						}
					});
				}
			);
		}
};

auto main(int ac, char **av) -> int
{
	ES::Engine::Core core;

#if defined(ES_DEBUG)
	spdlog::set_level(spdlog::level::debug);
#endif

	core.AddPlugins<
		ES::Plugin::WebGPU::Plugin,
		ES::Plugin::ImGUI::WebGPU::Plugin,
		CameraPlugin
	>();

	core.RegisterSystem<ES::Engine::Scheduler::Startup>(
		[](ES::Engine::Core &core) {
			auto &cameraData = core.GetResource<CameraData>();
			auto &window = core.GetResource<ES::Plugin::Window::Resource::Window>();

			auto size = window.GetSize();

			cameraData.position = { -600.0f, 300.0f, 0.0f };
			cameraData.pitch = glm::radians(-35.0f);
			cameraData.aspectRatio = static_cast<float>(size.x) / static_cast<float>(size.y);
		},
		[](ES::Engine::Core &core) {
			auto &lights = core.GetResource<std::vector<Light>>();

			lights.clear();

			lights.push_back({
				.color = { 204.0f / 255.0f, 42.0f / 255.0f, 34.0f / 255.0f, 1.0f },
				.direction = { 0.0f, 20.0f, 0.0f },
				.intensity = 250.f,
				.enabled = true
			});

			lights.push_back({
				// .color = { 0.1f, 0.9f, 0.3f, 1.0f }, 136, 255, 36
				.color = { 136.0f / 255.0f, 255.0f / 255.0f, 36.0f / 255.0f, 1.0f },
				.direction = { -300.0f, 10.0f, 0.0f },
				.intensity = 100.f,
				.enabled = true
			});

			lights.push_back({
				.color = { 0.0f / 255.0f, 86.0f / 255.0f, 255.0f / 255.0f, 1.0f },
				.direction = { 300.0f, 200.0f, 150.0f },
				.intensity = 490.f,
				.enabled = true
			});

			lights.push_back({
				.color = { 255.0f / 255.0f, 134.0f / 255.0f, 82.0f / 255.0f, 1.0f },
				.direction = { -125.0f, 55.0f, 32.0f },
				.intensity = 0.4f,
				.enabled = true,
				.type = Light::Type::Directional
			});

			ES::Plugin::WebGPU::Util::UpdateLights(core);
		}
	);

	core.RegisterSystem<ES::Engine::Scheduler::Startup>(
		[](ES::Engine::Core &core) {
			auto &inputManager = core.GetResource<ES::Plugin::Input::Resource::InputManager>();
			inputManager.RegisterKeyCallback([](ES::Engine::Core &cbCore, int key, int, int action, int) {
				if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
					cbCore.Stop();
				}
			});
		},
		[](ES::Engine::Core &core) {
			auto entity = ES::Engine::Entity(core.CreateEntity());

			std::vector<glm::vec3> vertices;
			std::vector<glm::vec3> normals;
			std::vector<glm::vec2> texCoords;
			std::vector<uint32_t> indices;

			bool success = ES::Plugin::Object::Resource::OBJLoader::loadModel("assets/sponza.obj", vertices, normals, texCoords, indices);
			if (!success) throw std::runtime_error("Model cant be loaded");

			entity.AddComponent<ES::Plugin::WebGPU::Component::Mesh>(core, core, vertices, normals, texCoords, indices).pipelineName = "3D";
			entity.AddComponent<ES::Plugin::Object::Component::Transform>(core, glm::vec3(0.0f, 0.0f, 0.0f));
			entity.AddComponent<Name>(core, "Sponza");
		},
		[](ES::Engine::Core &core) {
			auto entity = ES::Engine::Entity(core.CreateEntity());

			std::vector<glm::vec3> vertices;
			std::vector<glm::vec3> normals;
			std::vector<glm::vec2> texCoords;
			std::vector<uint32_t> indices;

			bool success = ES::Plugin::Object::Resource::OBJLoader::loadModel("assets/finish.obj", vertices, normals, texCoords, indices);
			if (!success) throw std::runtime_error("Model cant be loaded");

			entity.AddComponent<ES::Plugin::WebGPU::Component::Mesh>(core, core, vertices, normals, texCoords, indices).pipelineName = "3D";
			entity.AddComponent<ES::Plugin::Object::Component::Transform>(core, glm::vec3(0.0f, 0.0f, 0.0f));
			entity.AddComponent<Name>(core, "Finish");
		},
		[](ES::Engine::Core &core) {
			auto entity = ES::Engine::Entity(core.CreateEntity());

			auto &textureManager = core.GetResource<TextureManager>();
			auto &pipelines = core.GetResource<Pipelines>();
			textureManager.Add(entt::hashed_string("sprite_example"), core.GetResource<wgpu::Device>(), "./assets/insect.png", pipelines.renderPipelines["2D"].bindGroupLayouts[1]);

			std::vector<glm::vec3> vertices;
			std::vector<glm::vec3> normals;
			std::vector<glm::vec2> texCoords;
			std::vector<uint32_t> indices;

			ES::Plugin::WebGPU::Util::CreateSprite(glm::vec2(-50.f, -100.f), glm::vec2(284.0f, 372.0f), vertices, normals, texCoords, indices);

			auto &mesh = entity.AddComponent<ES::Plugin::WebGPU::Component::Mesh>(core, core, vertices, normals, texCoords, indices);
			mesh.pipelineName = "2D";
			mesh.textures.push_back(entt::hashed_string("sprite_example"));
			entity.AddComponent<ES::Plugin::Object::Component::Transform>(core, glm::vec3(0.0f, 0.0f, 0.0f));
			entity.AddComponent<Name>(core, "Sprite Example");
		},
		[](ES::Engine::Core &core) {
			auto entity = ES::Engine::Entity(core.CreateEntity());

			auto &textureManager = core.GetResource<TextureManager>();
			auto &pipelines = core.GetResource<Pipelines>();
			textureManager.Add(entt::hashed_string("sprite_example_2"), core.GetResource<wgpu::Device>(), glm::uvec2(200, 200), [](glm::uvec2 pos) {
				glm::u8vec4 color;
				if (pos.x >= 40 && pos.x <= 160 && pos.y >= 40 && pos.y <= 160) {
					return glm::u8vec4(0, 0, 0, 0);
				}
				color.r = (pos.x / 16) % 2 == (pos.y / 16) % 2 ? 255 : 0; // r
				color.g = ((pos.x - pos.y) / 16) % 2 == 0 ? 255 : 0; // g
				color.b = ((pos.x + pos.y) / 16) % 2 == 0 ? 255 : 0; // b
				color.a = 255; // a
				return color;
			}, pipelines.renderPipelines["2D"].bindGroupLayouts[1]);

			std::vector<glm::vec3> vertices;
			std::vector<glm::vec3> normals;
			std::vector<glm::vec2> texCoords;
			std::vector<uint32_t> indices;

			ES::Plugin::WebGPU::Util::CreateSprite(glm::vec2(0.f, -350.f), glm::vec2(200.0f, 200.0f), vertices, normals, texCoords, indices);

			auto &mesh = entity.AddComponent<ES::Plugin::WebGPU::Component::Mesh>(core, core, vertices, normals, texCoords, indices);
			mesh.pipelineName = "2D";
			mesh.textures.push_back(entt::hashed_string("sprite_example_2"));
			entity.AddComponent<ES::Plugin::Object::Component::Transform>(core, glm::vec3(0.0f, 0.0f, 0.0f));
			entity.AddComponent<Name>(core, "Sprite Example 2");
		}
	);

	core.RunCore();

	return 0;
}
