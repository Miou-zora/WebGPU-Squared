#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#include <glm/ext.hpp>

#define WEBGPU_CPP_IMPLEMENTATION
#include "webgpu.hpp"
#include <glfw3webgpu.h>
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
#include "structs.hpp"

#include <imgui.h>
#include <backends/imgui_impl_wgpu.h>
#include <backends/imgui_impl_glfw.h>

#include "Mesh.hpp"
#include "utils.hpp"
#include "RequestAdapterSync.hpp"
#include "AdaptaterPrint.hpp"
#include "RequestDeviceSync.hpp"
#include "InspectDevice.hpp"
#include "InitBuffers.hpp"
#include "InitializePipeline.hpp"
#include "CreateSurface.hpp"
#include "CreateInstance.hpp"
#include "CreateAdapter.hpp"
#include "ReleaseInstance.hpp"
#include "RequestCapabilities.hpp"
#include "CreateDevice.hpp"
#include "CreateQueue.hpp"
#include "SetupQueueOnSubmittedWorkDone.hpp"
#include "ConfigureSurface.hpp"
#include "ReleaseAdapter.hpp"
#include "CreateBindingGroup.hpp"
#include "GetNextSurfaceViewData.hpp"
#include "Clear.hpp"
#include "DrawWGPU.hpp"
#include "ReleasePipeline.hpp"
#include "ReleaseBuffers.hpp"
#include "ReleaseDevice.hpp"
#include "ReleaseSurface.hpp"
#include "ReleaseQueue.hpp"
#include "ReleaseUniforms.hpp"
#include "ReleaseBindingGroup.hpp"
#include "TerminateDepthBuffer.hpp"
#include "InitDepthBuffer.hpp"
#include "UnconfigureSurface.hpp"
#include "SetupResizableWindow.hpp"

// TODO: check learn webgpu c++ why I had this variable
uint32_t uniformStride = 0;

float cameraScale = 100.0f;

wgpu::TextureView customTextureView;

void Initialize2DPipeline(ES::Engine::Core &core)
{
	auto &device = core.GetResource<wgpu::Device>();
	auto &surface = core.GetResource<wgpu::Surface>();
	auto &window = core.GetResource<ES::Plugin::Window::Resource::Window>();
	wgpu::TextureFormat surfaceFormat = core.GetResource<wgpu::SurfaceCapabilities>().formats[0];


	if (device == nullptr) throw std::runtime_error("WebGPU device is not created, cannot initialize 2D pipeline.");
	if (surface == nullptr) throw std::runtime_error("WebGPU surface is not created, cannot initialize 2D pipeline.");

	wgpu::RenderPipelineDescriptor pipelineDesc(wgpu::Default);
	pipelineDesc.label = wgpu::StringView("2D Render Pipeline");

	wgpu::ShaderSourceWGSL wgslDesc(wgpu::Default);
	std::string wgslSource = loadFile("shader2D.wgsl");
	wgslDesc.code = wgpu::StringView(wgslSource);
	wgpu::ShaderModuleDescriptor shaderDesc(wgpu::Default);
    shaderDesc.nextInChain = &wgslDesc.chain; // connect the chained extension
    shaderDesc.label = wgpu::StringView("Shader source from Application.cpp");
	wgpu::ShaderModule shaderModule = device.createShaderModule(shaderDesc);
	wgpu::VertexBufferLayout vertexBufferLayout(wgpu::Default);

	std::vector<wgpu::VertexAttribute> vertexAttribs(2);

    // Describe the position attribute
    vertexAttribs[0].shaderLocation = 0;
    vertexAttribs[0].format = wgpu::VertexFormat::Float32x3;
    vertexAttribs[0].offset = 0;

	vertexAttribs[1].shaderLocation = 1;
	vertexAttribs[1].format = wgpu::VertexFormat::Float32x2;
	vertexAttribs[1].offset = 3 * sizeof(float); // Offset by the size of the position attribute

    vertexBufferLayout.attributeCount = static_cast<uint32_t>(vertexAttribs.size());
    vertexBufferLayout.attributes = vertexAttribs.data();

    vertexBufferLayout.arrayStride = (3 * sizeof(float)) + (2 * sizeof(float));
    vertexBufferLayout.stepMode = wgpu::VertexStepMode::Vertex;

		// TODO: find why it does not work with wgpu::BindGroupLayoutEntry
	WGPUBindGroupLayoutEntry bindingLayout = {0};
	bindingLayout.binding = 0;
	bindingLayout.visibility = wgpu::ShaderStage::Vertex;
	bindingLayout.buffer.type = wgpu::BufferBindingType::Uniform;
	bindingLayout.buffer.minBindingSize = sizeof(Uniforms2D);

	WGPUBindGroupLayoutEntry textureBindingLayout = {0};
	textureBindingLayout.binding = 1;
	textureBindingLayout.visibility = wgpu::ShaderStage::Fragment;
	textureBindingLayout.texture.sampleType = wgpu::TextureSampleType::Float;
	textureBindingLayout.texture.viewDimension = wgpu::TextureViewDimension::_2D;

	std::array<WGPUBindGroupLayoutEntry, 2> bindings = { bindingLayout, textureBindingLayout };

	wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc(wgpu::Default);
	bindGroupLayoutDesc.entryCount = bindings.size();
	bindGroupLayoutDesc.entries = bindings.data();
	bindGroupLayoutDesc.label = wgpu::StringView("My Bind Group Layout");
	wgpu::BindGroupLayout bindGroupLayout = device.createBindGroupLayout(bindGroupLayoutDesc);

	wgpu::PipelineLayoutDescriptor layoutDesc(wgpu::Default);
	layoutDesc.bindGroupLayoutCount = 1;
	layoutDesc.bindGroupLayouts = reinterpret_cast<WGPUBindGroupLayout*>(&bindGroupLayout);
	wgpu::PipelineLayout layout = device.createPipelineLayout(layoutDesc);

	pipelineDesc.vertex.bufferCount = 1;
	pipelineDesc.vertex.buffers = &vertexBufferLayout;
	pipelineDesc.vertex.module = shaderModule;
	pipelineDesc.vertex.entryPoint = wgpu::StringView("vs_main");

	wgpu::FragmentState fragmentState(wgpu::Default);
    fragmentState.module = shaderModule;
	fragmentState.entryPoint = wgpu::StringView("fs_main");

	wgpu::ColorTargetState colorTarget(wgpu::Default);
    colorTarget.format = surfaceFormat;
	colorTarget.writeMask = wgpu::ColorWriteMask::All;

	wgpu::BlendState blendState(wgpu::Default);
    colorTarget.blend = &blendState;
    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTarget;
    pipelineDesc.fragment = &fragmentState;
	pipelineDesc.layout = layout;

	int frameBufferSizeX, frameBufferSizeY;
	glfwGetFramebufferSize(window.GetGLFWWindow(), &frameBufferSizeX, &frameBufferSizeY);

	wgpu::DepthStencilState depthStencilState(wgpu::Default);
	depthStencilState.depthCompare = wgpu::CompareFunction::Less;
	depthStencilState.depthWriteEnabled = wgpu::OptionalBool::True;
	depthStencilState.format = depthTextureFormat;
	pipelineDesc.depthStencil = &depthStencilState;

	wgpu::RenderPipeline pipeline = device.createRenderPipeline(pipelineDesc);

	if (pipeline == nullptr) throw std::runtime_error("Could not create render pipeline");

	wgpuShaderModuleRelease(shaderModule);

	core.GetResource<Pipelines>().renderPipelines["2D"] = PipelineData{
		.pipeline = pipeline,
		.bindGroupLayout = bindGroupLayout,
		.layout = layout,
	};
}

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

void CreateBindingGroup2D(ES::Engine::Core &core)
{
	auto &device = core.GetResource<wgpu::Device>();
	auto &pipelineData = core.GetResource<Pipelines>().renderPipelines["2D"];
	auto &bindGroups = core.GetResource<BindGroups>();
	//TODO: Put this in a separate system
	//TODO: Should we separate this from pipelineData?

	if (device == nullptr) throw std::runtime_error("WebGPU device is not created, cannot create binding group.");

	wgpu::BindGroupEntry binding(wgpu::Default);
	binding.binding = 0;
	binding.buffer = uniform2DBuffer;
	binding.size = sizeof(Uniforms2D);

	wgpu::BindGroupEntry textureBinding(wgpu::Default);
	textureBinding.binding = 1;
	textureBinding.textureView = customTextureView;

	std::array<wgpu::BindGroupEntry, 2> bindings = { binding, textureBinding };

	wgpu::BindGroupDescriptor bindGroupDesc(wgpu::Default);
	bindGroupDesc.layout = pipelineData.bindGroupLayout;
	bindGroupDesc.entryCount = bindings.size();
	bindGroupDesc.entries = bindings.data();
	bindGroupDesc.label = wgpu::StringView("My Bind Group");
	auto bg1 = device.createBindGroup(bindGroupDesc);

	if (bg1 == nullptr) throw std::runtime_error("Could not create WebGPU bind group");

	bindGroups.groups["2D"] = bg1;
}

void GenerateSurfaceTexture(ES::Engine::Core &core)
{
	wgpu::Surface &surface = core.GetResource<wgpu::Surface>();
	textureView = GetNextSurfaceViewData(surface);
	if (textureView == nullptr) throw std::runtime_error("Could not get next surface texture view");
}


void CreateTexture(ES::Engine::Core &core)
{
	auto &device = core.GetResource<wgpu::Device>();
	auto &queue = core.GetResource<wgpu::Queue>();

	wgpu::TextureDescriptor textureDesc;
	textureDesc.label = wgpu::StringView("Custom Texture");
	textureDesc.size = { 100, 200, 1 };
	textureDesc.dimension = wgpu::TextureDimension::_2D;
	textureDesc.mipLevelCount = 1;
	textureDesc.sampleCount = 1;
	textureDesc.format = wgpu::TextureFormat::RGBA8Unorm;
	textureDesc.usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst;
	textureDesc.viewFormats = nullptr;
	textureDesc.viewFormatCount = 0;
	wgpu::Texture texture = device.createTexture(textureDesc);

	wgpu::TextureViewDescriptor textureViewDesc;
	textureViewDesc.aspect = wgpu::TextureAspect::All;
	textureViewDesc.baseArrayLayer = 0;
	textureViewDesc.arrayLayerCount = 1;
	textureViewDesc.baseMipLevel = 0;
	textureViewDesc.mipLevelCount = 1;
	textureViewDesc.dimension = wgpu::TextureViewDimension::_2D;
	textureViewDesc.format = textureDesc.format;
	customTextureView = texture.createView(textureViewDesc);

	wgpu::TexelCopyTextureInfo destination;
	destination.texture = texture;
	destination.mipLevel = 0;
	destination.origin = { 0, 0, 0 }; // equivalent of the offset argument of Queue::writeBuffer
	destination.aspect = wgpu::TextureAspect::All;

	wgpu::TexelCopyBufferLayout source;
	source.offset = 0;
	source.bytesPerRow = 4 * textureDesc.size.width;
	source.rowsPerImage = textureDesc.size.height;

	std::vector<uint8_t> pixels(4 * textureDesc.size.width * textureDesc.size.height);
	for (uint32_t i = 0; i < textureDesc.size.width; ++i) {
		for (uint32_t j = 0; j < textureDesc.size.height; ++j) {
			uint8_t *p = &pixels[4 * (j * textureDesc.size.width + i)];
			p[0] = (i / 16) % 2 == (j / 16) % 2 ? 255 : 0; // r
			p[1] = ((i - j) / 16) % 2 == 0 ? 255 : 0; // g
			p[2] = ((i + j) / 16) % 2 == 0 ? 255 : 0; // b
			p[3] = 255; // a
		}
	}

	queue.writeTexture(destination, pixels.data(), pixels.size(), source, textureDesc.size);
}

namespace ES::Plugin::WebGPU {
class Plugin : public ES::Engine::APlugin {
  public:
    using APlugin::APlugin;
    ~Plugin() = default;

    void Bind() final {
		RequirePlugins<ES::Plugin::Window::Plugin>();

		RegisterResource<ClearColor>(ClearColor());
		RegisterResource<Pipelines>(Pipelines());

		RegisterSystems<ES::Plugin::RenderingPipeline::Setup>(
			CreateInstance,
			CreateSurface,
			CreateAdapter,
			AdaptaterPrintLimits,
			AdaptaterPrintFeatures,
			AdaptaterPrintProperties,
			ReleaseInstance,
			RequestCapabilities,
			CreateDevice,
			CreateQueue,
			SetupQueueOnSubmittedWorkDone,
			ConfigureSurface,
			ReleaseAdapter,
			InspectDevice,
			InitDepthBuffer,
			InitializePipeline,
			Initialize2DPipeline,
			InitializeBuffers,
			Create2DPipelineBuffer,
			CreateBindingGroup,
			CreateTexture,
			CreateBindingGroup2D,
			SetupResizableWindow
		);
		RegisterSystems<ES::Plugin::RenderingPipeline::Draw>(
			GenerateSurfaceTexture,
			Clear,
			DrawMeshes
		);
		RegisterSystems<ES::Engine::Scheduler::Shutdown>(
			ReleaseBindingGroup,
			ReleaseUniforms,
			ReleaseBuffers,
			ReleasePipeline,
			TerminateDepthBuffer,
			ReleaseDevice,
			ReleaseSurface,
			ReleaseQueue
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
	//TODO: check why X axis is inverted
    if (inputManager.IsKeyPressed(GLFW_KEY_A)) {
        force.x += 1.0f;
    }
    if (inputManager.IsKeyPressed(GLFW_KEY_D)) {
        force.x -= 1.0f;
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

void MovementSystem(ES::Engine::Core &core)
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

auto main(int ac, char **av) -> int
{
	ES::Engine::Core core;

	core.AddPlugins<ES::Plugin::WebGPU::Plugin, ES::Plugin::Input::Plugin>();

	core.RegisterSystem<ES::Plugin::RenderingPipeline::Setup>([](ES::Engine::Core &core) {
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
	},
	[](ES::Engine::Core &core) {
		auto &lights = core.GetResource<std::vector<Light>>();

		lights.at(0) = {
			.color = { 0.8f, 0.2f, 0.2f, 1.0f },
			.direction = { 200.0f, 100.0f, 0.0f },
			.intensity = 100.f,
			.enabled = true
		};

		lights.at(1) = {
			.color = { 0.1f, 0.9f, 0.3f, 1.0f },
			.direction = { -300.0f, 100.0f, 0.0f },
			.intensity = 100.f,
			.enabled = true
		};

		lights.at(2) = {
			.color = { 0.0f, 0.0f, 1.0f, 1.0f },
			.direction = { 0.0f, 100.0f, 300.0f },
			.intensity = 100.f,
			.enabled = true
		};
	});

	// TODO: avoid defining the camera data in the main.cpp, use default values
	core.RegisterResource<CameraData>({
		.position = { 0.0f, 100.0f, 0.0f },
		.yaw = 4.75f,
		.pitch = -0.75f,
		.up = { 0.0f, 1.0f, 0.0f },
		.fovY = glm::radians(45.0f),
		.nearPlane = 10.f,
		.farPlane = 10000.0f,
		.aspectRatio = 800.0f / 800.0f
	});

	core.RegisterResource<DragState>({
    	.active = false,
		.startMouse = { 0.0f, 0.0f },
		.originYaw = 0.0f,
		.originPitch = 0.0f,
		.sensitivity = 0.005f,
		.scrollSensitivity = 0.1f
	});

	core.RegisterSystem(MovementSystem,
	[](ES::Engine::Core &core) {
		auto &cameraData = core.GetResource<CameraData>();
		cameraData.farPlane = 100.0f * cameraScale;
		cameraData.nearPlane = 0.1f * cameraScale;
	});

	core.RegisterSystem<ES::Engine::Scheduler::FixedTimeUpdate>([](ES::Engine::Core &core) {
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
	});

	core.RegisterSystem<ES::Engine::Scheduler::Startup>([&](ES::Engine::Core &core) {
		auto &window = core.GetResource<ES::Plugin::Window::Resource::Window>();
		auto &inputManager = core.GetResource<ES::Plugin::Input::Resource::InputManager>();

		inputManager.RegisterScrollCallback([&](ES::Engine::Core &, double, double y) {
			auto &cameraData = core.GetResource<CameraData>();
			static float sensitivity = 0.01f;
			cameraData.fovY += sensitivity * static_cast<float>(-y);
			cameraData.fovY = glm::clamp(cameraData.fovY, glm::radians(0.1f), glm::radians(179.9f));
		});

		inputManager.RegisterKeyCallback([](ES::Engine::Core &cbCore, int key, int scancode, int action, int mods) {
			if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
				cbCore.Stop();
			}

			ImGuiIO& io = ImGui::GetIO();
			if (io.WantCaptureMouse) {
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
			ImGuiIO& io = ImGui::GetIO();
			if (io.WantCaptureMouse) {
				ImGui_ImplGlfw_MouseButtonCallback(cbCore.GetResource<ES::Plugin::Window::Resource::Window>().GetGLFWWindow(), button, action, 0);
				return;
			}
			auto &cameraData = cbCore.GetResource<CameraData>();
			auto &drag = cbCore.GetResource<DragState>();
			auto &window = cbCore.GetResource<ES::Plugin::Window::Resource::Window>();
			glm::vec2 mousePos = window.GetMousePosition();
			if (button == GLFW_MOUSE_BUTTON_LEFT) {
				switch(action) {
				case GLFW_PRESS:
					drag.active = true;
					drag.startMouse = glm::vec2(-mousePos.x, -window.GetSize().y+mousePos.y);
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
				glm::vec2 currentMouse = glm::vec2(-(float)x, -(float)y);
				glm::vec2 delta = (currentMouse - drag.startMouse) * drag.sensitivity;
				cameraData.yaw = drag.originYaw + delta.x;
				cameraData.pitch = drag.originPitch + delta.y;
				cameraData.pitch = glm::clamp(cameraData.pitch, -glm::half_pi<float>() + 1e-5f, glm::half_pi<float>() - 1e-5f);
				drag.velocity = delta - drag.previousDelta;
        		drag.previousDelta = delta;
			}
		});
	},
	[&](ES::Engine::Core &core) {
		auto entity = ES::Engine::Entity(core.CreateEntity());

		std::vector<glm::vec3> vertices;
		std::vector<glm::vec3> normals;
		std::vector<glm::vec2> texCoords;
		std::vector<uint32_t> indices;

		bool success = ES::Plugin::Object::Resource::OBJLoader::loadModel("assets/sponza.obj", vertices, normals, texCoords, indices);
		if (!success) throw std::runtime_error("Model cant be loaded");

		entity.AddComponent<Mesh>(core, core, vertices, normals, indices);
		entity.AddComponent<ES::Plugin::Object::Component::Transform>(core, glm::vec3(0.0f, 0.0f, 0.0f));
		entity.AddComponent<Name>(core, "Sponza");
	},
	[&](ES::Engine::Core &core) {
		auto entity = ES::Engine::Entity(core.CreateEntity());

		std::vector<glm::vec3> vertices;
		std::vector<glm::vec3> normals;
		std::vector<glm::vec2> texCoords;
		std::vector<uint32_t> indices;

		bool success = ES::Plugin::Object::Resource::OBJLoader::loadModel("assets/finish.obj", vertices, normals, texCoords, indices);
		if (!success) throw std::runtime_error("Model cant be loaded");

		entity.AddComponent<Mesh>(core, core, vertices, normals, indices);
		entity.AddComponent<ES::Plugin::Object::Component::Transform>(core, glm::vec3(0.0f, 0.0f, 0.0f));
		entity.AddComponent<Name>(core, "Finish");
	},
	[&](ES::Engine::Core &core) {
		auto entity = ES::Engine::Entity(core.CreateEntity());

		entity.AddComponent<Sprite>(core, core, glm::vec2(-50.f, -100.f), glm::vec2(100.0f, 200.0f));
		entity.AddComponent<Name>(core, "Sprite Example");
	});

	core.RunCore();

	return 0;
}
