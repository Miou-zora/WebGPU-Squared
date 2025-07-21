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
#include <GLFW/glfw3.h>
#include "RenderingPipeline.hpp"

#include <imgui.h>
#include "imgui_impl_wgpu.h"
#include "imgui_impl_glfw.h"


struct MyUniforms {
	glm::mat4x4 projectionMatrix;
    glm::mat4x4 viewMatrix;
    glm::mat4x4 modelMatrix;
    glm::vec4 color;
    float time;
    float _pad[3];
};

// This assert should stay here as we want this rule to link struct to webgpu struct
static_assert(sizeof(MyUniforms) % 16 == 0);

struct CameraData {
	glm::vec3 position;
	float yaw; // Yaw angle in radians
	float pitch; // Pitch angle in radians
	glm::vec3 up;
	float fovY; // Field of view in radians
	float nearPlane;
	float farPlane;
	float aspectRatio;
};


struct Mesh {
	wgpu::Buffer pointBuffer = nullptr;
	wgpu::Buffer indexBuffer = nullptr;
	uint32_t indexCount = 0;

	bool enabled = true;

	Mesh() = default;

	Mesh(ES::Engine::Core &core, const std::vector<glm::vec3> &vertices, const std::vector<glm::vec3> &normals, const std::vector<uint32_t> &indices) {
		auto &device = core.GetResource<wgpu::Device>();
		auto &queue = core.GetResource<wgpu::Queue>();

		std::vector<float> pointData;
		for (size_t i = 0; i < vertices.size(); i++) {
			pointData.push_back(vertices.at(i).x);
			pointData.push_back(vertices.at(i).y);
			pointData.push_back(vertices.at(i).z);
			pointData.push_back(normals.at(i).r);
			pointData.push_back(normals.at(i).g);
			pointData.push_back(normals.at(i).b);
		}


		std::vector<uint32_t> indexData;
		for (size_t i = 0; i < indices.size(); i++) {
			indexData.push_back(indices.at(i));
		}

		indexCount = indexData.size();

		wgpu::BufferDescriptor bufferDesc(wgpu::Default);
		bufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex;
		bufferDesc.mappedAtCreation = false;
		bufferDesc.size = pointData.size() * sizeof(float);
		bufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex;
		pointBuffer = device.createBuffer(bufferDesc);

		queue.writeBuffer(pointBuffer, 0, pointData.data(), bufferDesc.size);

		bufferDesc.size = indexData.size() * sizeof(uint32_t);
		bufferDesc.size = (bufferDesc.size + 3) & ~3;
		indexData.resize((indexData.size() + 1) & ~1);
		bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index;
		indexBuffer = device.createBuffer(bufferDesc);

		queue.writeBuffer(indexBuffer, 0, indexData.data(), bufferDesc.size);
	}

	void Release() {
		if (pointBuffer) {
			pointBuffer.destroy();
			pointBuffer.release();
			pointBuffer = nullptr;
		}
		if (indexBuffer) {
			indexBuffer.destroy();
			indexBuffer.release();
			indexBuffer = nullptr;
		}
		indexCount = 0;
	}
};

struct DragState {
    // Whether a drag action is ongoing (i.e., we are between mouse press and mouse release)
    bool active = false;
    // The position of the mouse at the beginning of the drag action
    glm::vec2 startMouse;
    // The camera state at the beginning of the drag action
    float originYaw = 0.0f;
    float originPitch = 0.0f;

    // Constant settings
    float sensitivity = 0.01f;
    float scrollSensitivity = 0.1f;
	glm::vec2 velocity = {0.0, 0.0};
    glm::vec2 previousDelta = {0.0, 0.0};
    float inertia = 0.9f;
};

struct ClearColor {
	wgpu::Color value = { 0.05, 0.05, 0.05, 1.0 };
};

struct Name {
	std::string value;
};

uint32_t ceilToNextMultiple(uint32_t value, uint32_t step) {
    uint32_t divide_and_ceil = value / step + (value % step == 0 ? 0 : 1);
    return step * divide_and_ceil;
}

std::string_view toStdStringView(wgpu::StringView wgpuStringView) {
    return
        wgpuStringView.data == nullptr
        ? std::string_view()
        : wgpuStringView.length == WGPU_STRLEN
        ? std::string_view(wgpuStringView.data)
        : std::string_view(wgpuStringView.data, wgpuStringView.length);
}

auto requestAdapterSync(wgpu::Instance instance, wgpu::RequestAdapterOptions const &options)
	-> wgpu::Adapter
{
    struct UserData {
        wgpu::Adapter adapter = nullptr;
        bool requestEnded = false;
    };
    UserData userData;

    auto onAdapterRequestEnded = [](
		WGPURequestAdapterStatus status,
		WGPUAdapter adapter,
		WGPUStringView message,
		WGPU_NULLABLE void* userdata1,
		WGPU_NULLABLE void* userdata2
	) {
        UserData& userData = *reinterpret_cast<UserData*>(userdata1);
		userData.requestEnded = true;
        if (status == wgpu::RequestAdapterStatus::Success) {
			userData.adapter = adapter;
		} else {
			userData.adapter = nullptr;
			ES::Utils::Log::Error(fmt::format("Could not get WebGPU adapter: {}", toStdStringView(message)));
		}
    };

	// TODO: Use proper struct initialization
    wgpu::RequestAdapterCallbackInfo callbackInfo(wgpu::Default);
    callbackInfo.callback = onAdapterRequestEnded;
    callbackInfo.userdata1 = &userData;

	instance.requestAdapter(wgpu::Default, callbackInfo);

    while (!userData.requestEnded) {
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}

    return userData.adapter;
}

void AdaptaterPrintLimits(ES::Engine::Core &core)
{
	const auto &adapter = core.GetResource<wgpu::Adapter>();
	if (adapter == nullptr) throw std::runtime_error("Adapter is null, cannot print limits.");

	// TODO: Use proper struct initialization
	wgpu::Limits supportedLimits(wgpu::Default);

	wgpu::Status success = adapter.getLimits(&supportedLimits);

	if (success != wgpu::Status::Success) throw std::runtime_error("Failed to get adapter limits");

	ES::Utils::Log::Info("Adapter limits:");
	ES::Utils::Log::Info(fmt::format(" - maxTextureDimension1D: {}", supportedLimits.maxTextureDimension1D));
	ES::Utils::Log::Info(fmt::format(" - maxTextureDimension2D: {}", supportedLimits.maxTextureDimension2D));
	ES::Utils::Log::Info(fmt::format(" - maxTextureDimension3D: {}", supportedLimits.maxTextureDimension3D));
	ES::Utils::Log::Info(fmt::format(" - maxTextureArrayLayers: {}", supportedLimits.maxTextureArrayLayers));
}

void AdaptaterPrintFeatures(ES::Engine::Core &core)
{
	const auto &adapter = core.GetResource<wgpu::Adapter>();
	if (adapter == nullptr) throw std::runtime_error("Adapter is null, cannot print features.");

	wgpu::SupportedFeatures features(wgpu::Default);

	adapter.getFeatures(&features);

	ES::Utils::Log::Info("Adapter features:");
	for (size_t i = 0; i < features.featureCount; ++i) {
		ES::Utils::Log::Info(fmt::format(" - 0x{:x}", static_cast<uint32_t>(features.features[i])));
	}

	features.freeMembers();
}

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

	ImGui::End();

    // Draw the UI
    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), renderPass);
}

void AdaptaterPrintProperties(ES::Engine::Core &core)
{
	const auto &adapter = core.GetResource<wgpu::Adapter>();
	if (adapter == nullptr) throw std::runtime_error("Adapter is null, cannot print properties.");

	// TODO: Use proper struct initialization
	wgpu::AdapterInfo properties(wgpu::Default);

	adapter.getInfo(&properties);

	ES::Utils::Log::Info("Adapter properties:");
	ES::Utils::Log::Info(fmt::format(" - vendorID: {}", properties.vendorID));
	ES::Utils::Log::Info(fmt::format(" - vendorName: {}", toStdStringView(properties.vendor)));
	ES::Utils::Log::Info(fmt::format(" - architecture: {}", toStdStringView(properties.architecture)));
	ES::Utils::Log::Info(fmt::format(" - deviceID: {}", properties.deviceID));
	ES::Utils::Log::Info(fmt::format(" - name: {}", toStdStringView(properties.device)));
	ES::Utils::Log::Info(fmt::format(" - driverDescription: {}", toStdStringView(properties.description)));
	ES::Utils::Log::Info(fmt::format(" - adapterType: 0x{:x}", static_cast<uint32_t>(properties.adapterType)));
	ES::Utils::Log::Info(fmt::format(" - backendType: 0x{:x}", static_cast<uint32_t>(properties.backendType)));

	properties.freeMembers();
}

wgpu::Device requestDeviceSync(const wgpu::Adapter &adapter, wgpu::DeviceDescriptor const &descriptor) {
    struct UserData {
        wgpu::Device device = nullptr;
        bool requestEnded = false;
    };
    UserData userData;

    auto onDeviceRequestEnded = []
	(WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message, WGPU_NULLABLE void* userdata1, WGPU_NULLABLE void* userdata2) {
        UserData& userData = *reinterpret_cast<UserData*>(userdata1);
        if (status == wgpu::RequestDeviceStatus::Success) {
            userData.device = device;
        } else {
			userData.device = nullptr;
			ES::Utils::Log::Error(fmt::format("Could not get WebGPU device: {}", toStdStringView(message)));
		}
        userData.requestEnded = true;
    };

	wgpu::RequestDeviceCallbackInfo callbackInfo(wgpu::Default);
    callbackInfo.callback = onDeviceRequestEnded;
    callbackInfo.userdata1 = &userData;

    adapter.requestDevice(
        descriptor,
        callbackInfo
    );

	if (!userData.requestEnded) {
		ES::Utils::Log::Error("Request device timed out");
		throw std::runtime_error("Request device timed out");
	}

    return userData.device;
}

uint32_t uniformStride = 0;

void InspectDevice(ES::Engine::Core &core) {
    const wgpu::Device &device = core.GetResource<wgpu::Device>();
    if (device == nullptr) throw std::runtime_error("Device is not created, cannot inspect it.");

    wgpu::SupportedFeatures features(wgpu::Default);
    device.getFeatures(&features);

    ES::Utils::Log::Info("Device features:");
    for (size_t i = 0; i < features.featureCount; ++i) {
        ES::Utils::Log::Info(fmt::format(" - 0x{:x}", static_cast<uint32_t>(features.features[i])));
    }
    features.freeMembers();

    wgpu::Limits limits(wgpu::Default);
    limits.maxUniformBufferBindingSize = 16 * 4 * sizeof(float);
	limits.maxBindGroups = 2;

    bool success = device.getLimits(&limits) == wgpu::Status::Success;

	uniformStride = ceilToNextMultiple(
		(uint32_t)sizeof(MyUniforms),
		(uint32_t)limits.minUniformBufferOffsetAlignment
	);
	uniformStride = 0;

	if (!success) throw std::runtime_error("Failed to get device limits");

	ES::Utils::Log::Info("Device limits:");
	ES::Utils::Log::Info(fmt::format(" - maxTextureDimension1D: {}", limits.maxTextureDimension1D));
	ES::Utils::Log::Info(fmt::format(" - maxTextureDimension2D: {}", limits.maxTextureDimension2D));
	ES::Utils::Log::Info(fmt::format(" - maxTextureDimension3D: {}", limits.maxTextureDimension3D));
	ES::Utils::Log::Info(fmt::format(" - maxTextureArrayLayers: {}", limits.maxTextureArrayLayers));
}
// TODO: Release them
wgpu::Buffer uniformBuffer = nullptr;
wgpu::PipelineLayout layout = nullptr;
wgpu::BindGroupLayout bindGroupLayout = nullptr;
wgpu::TextureFormat depthTextureFormat = wgpu::TextureFormat::Depth24Plus;
wgpu::TextureView depthTextureView = nullptr;

void InitializeBuffers(ES::Engine::Core &core)
{
	wgpu::Queue &queue = core.GetResource<wgpu::Queue>();
	wgpu::Device &device = core.GetResource<wgpu::Device>();

	if (queue == nullptr) throw std::runtime_error("WebGPU queue is not created, cannot initialize buffers.");
	if (device == nullptr) throw std::runtime_error("WebGPU device is not created, cannot initialize buffers.");

	wgpu::BufferDescriptor bufferDesc(wgpu::Default);
	bufferDesc.size = sizeof(MyUniforms);
	bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
	uniformBuffer = device.createBuffer(bufferDesc);

	// Upload the initial value of the uniforms
	MyUniforms uniforms;
	uniforms.time = 1.0f;
	uniforms.color = { 0.0f, 1.0f, 0.4f, 1.0f };
	float near_value = 0.001f;
	float far_value = 100.0f;
	float ratio = 800.0f / 800.0f;
	float fov = glm::radians(45.0f);
	uniforms.projectionMatrix = glm::perspective(fov, ratio, near_value, far_value);
	queue.writeBuffer(uniformBuffer, 0, &uniforms, sizeof(uniforms));
}

std::string loadFile(const std::string &filePath) {
	std::ifstream file(filePath);
	if (!file.is_open()) {
		throw std::runtime_error("Could not open file: " + filePath);
	}
	std::stringstream buffer;
	buffer << file.rdbuf();
	file.close();
	return buffer.str();
}

void InitializePipeline(ES::Engine::Core &core)
{
	wgpu::Device device = core.GetResource<wgpu::Device>();
	wgpu::TextureFormat surfaceFormat = core.GetResource<wgpu::SurfaceCapabilities>().formats[0];

	if (device == nullptr) throw std::runtime_error("WebGPU device is not created, cannot initialize pipeline.");

	wgpu::ShaderSourceWGSL wgslDesc(wgpu::Default);
	std::string wgslSource = loadFile("shader.wgsl");
	wgslDesc.code = wgpu::StringView(wgslSource);

	wgpu::ShaderModuleDescriptor shaderDesc(wgpu::Default);
    shaderDesc.nextInChain = &wgslDesc.chain; // connect the chained extension
    shaderDesc.label = wgpu::StringView("Shader source from Application.cpp");

	wgpu::ShaderModule shaderModule = device.createShaderModule(shaderDesc);
	wgpu::RenderPipelineDescriptor pipelineDesc(wgpu::Default);
	pipelineDesc.label = wgpu::StringView("My Render Pipeline");
	wgpu::VertexBufferLayout vertexBufferLayout(wgpu::Default);

	std::vector<wgpu::VertexAttribute> vertexAttribs(2);

    // Describe the position attribute
    vertexAttribs[0].shaderLocation = 0;
    vertexAttribs[0].format = wgpu::VertexFormat::Float32x3;
    vertexAttribs[0].offset = 0;

    // Describe the color attribute
    vertexAttribs[1].shaderLocation = 1;
    vertexAttribs[1].format = wgpu::VertexFormat::Float32x3;
    vertexAttribs[1].offset = 3 * sizeof(float);

    vertexBufferLayout.attributeCount = static_cast<uint32_t>(vertexAttribs.size());
    vertexBufferLayout.attributes = vertexAttribs.data();

    vertexBufferLayout.arrayStride = 6 * sizeof(float);
    vertexBufferLayout.stepMode = wgpu::VertexStepMode::Vertex;

	// TODO: find why it does not work with wgpu::BindGroupLayoutEntry
	WGPUBindGroupLayoutEntry bindingLayout = {0};
	bindingLayout.binding = 0;
	bindingLayout.visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
	bindingLayout.buffer.type = wgpu::BufferBindingType::Uniform;
	bindingLayout.buffer.minBindingSize = sizeof(MyUniforms);

	wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc(wgpu::Default);
	bindGroupLayoutDesc.entryCount = 1;
	bindGroupLayoutDesc.entries = &bindingLayout;
	bindGroupLayoutDesc.label = wgpu::StringView("My Bind Group Layout");
	bindGroupLayout = device.createBindGroupLayout(bindGroupLayoutDesc);

	wgpu::PipelineLayoutDescriptor layoutDesc(wgpu::Default);
	layoutDesc.bindGroupLayoutCount = 1;
	layoutDesc.bindGroupLayouts = reinterpret_cast<WGPUBindGroupLayout*>(&bindGroupLayout);
	layout = device.createPipelineLayout(layoutDesc);

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
	pipelineDesc.label = wgpu::StringView("My Render Pipeline");
	pipelineDesc.layout = layout;

	auto &window = core.GetResource<ES::Plugin::Window::Resource::Window>();

	int frameBufferSizeX, frameBufferSizeY;
	glfwGetFramebufferSize(window.GetGLFWWindow(), &frameBufferSizeX, &frameBufferSizeY);

	wgpu::TextureDescriptor depthTextureDesc(wgpu::Default);
	depthTextureDesc.label = wgpu::StringView("Z Buffer");
	depthTextureDesc.usage = wgpu::TextureUsage::RenderAttachment;
	depthTextureDesc.size = { static_cast<uint32_t>(frameBufferSizeX), static_cast<uint32_t>(frameBufferSizeY), 1u };
	depthTextureDesc.format = depthTextureFormat;
	wgpu::Texture depthTexture = device.createTexture(depthTextureDesc);

	depthTextureView = depthTexture.createView();
	depthTexture.release();

	wgpu::DepthStencilState depthStencilState(wgpu::Default);
	depthStencilState.depthCompare = wgpu::CompareFunction::Less;
	depthStencilState.depthWriteEnabled = wgpu::OptionalBool::True;
	depthStencilState.format = depthTextureFormat;
	pipelineDesc.depthStencil = &depthStencilState;

	wgpu::RenderPipeline pipeline = core.RegisterResource(wgpu::Device(device).createRenderPipeline(pipelineDesc));

	if (pipeline == nullptr) throw std::runtime_error("Could not create render pipeline");

	wgpuShaderModuleRelease(shaderModule);
}

void CreateSurface(ES::Engine::Core &core) {
	ES::Utils::Log::Info("Creating surface...");

	auto &instance = core.GetResource<wgpu::Instance>();
	auto glfwWindow = core.GetResource<ES::Plugin::Window::Resource::Window>().GetGLFWWindow();

	wgpu::Surface &surface = core.RegisterResource(wgpu::Surface(glfwCreateWindowWGPUSurface(instance, glfwWindow)));

	glfwMakeContextCurrent(glfwWindow);

	if (surface == nullptr) throw std::runtime_error("Could not create surface");

	ES::Utils::Log::Info(fmt::format("Surface created: {}", static_cast<void*>(surface)));
}

void CreateInstance(ES::Engine::Core &core) {
	ES::Utils::Log::Info("Creating WebGPU instance...");

	wgpu::InstanceDescriptor desc(wgpu::Default);

	auto &instance = core.RegisterResource(wgpu::Instance(wgpuCreateInstance(&desc)));

	if (instance == nullptr) throw std::runtime_error("Could not create WebGPU instance");

	ES::Utils::Log::Info(fmt::format("WebGPU instance created: {}", static_cast<void*>(instance)));
}

void CreateAdapter(ES::Engine::Core &core) {
	ES::Utils::Log::Info("Requesting adapter...");

	auto &instance = core.GetResource<wgpu::Instance>();
	auto &surface = core.GetResource<wgpu::Surface>();

	if (instance == nullptr) throw std::runtime_error("WebGPU instance is not created, cannot request adapter");
	if (surface == nullptr) throw std::runtime_error("Surface is not created, cannot request adapter.");

	wgpu::RequestAdapterOptions adapterOpts(wgpu::Default);
	adapterOpts.compatibleSurface = surface;

	wgpu::Adapter adapter = core.RegisterResource(requestAdapterSync(instance, adapterOpts));

	if (adapter == nullptr) throw std::runtime_error("Could not get WebGPU adapter");

	ES::Utils::Log::Info(fmt::format("Got adapter: {}", static_cast<void*>(adapter)));
}

void ReleaseInstance(ES::Engine::Core &core) {
	wgpu::Instance &instance = core.GetResource<wgpu::Instance>();

	if (instance == nullptr) throw std::runtime_error("WebGPU instance is already released or was never created.");

	ES::Utils::Log::Info(fmt::format("Releasing WebGPU instance: {}", static_cast<void*>(instance)));

	instance.release();
	instance = nullptr;
	// TODO: Remove the instance from the core resources (#252)
}

void RequestCapabilities(ES::Engine::Core &core)
{
	const auto &adapter = core.GetResource<wgpu::Adapter>();
	const wgpu::Surface &surface = core.GetResource<wgpu::Surface>();

	if (adapter == nullptr) throw std::runtime_error("Adapter is not created, cannot request capabilities.");
	if (surface == nullptr) throw std::runtime_error("Surface is not created, cannot request capabilities.");

	wgpu::SurfaceCapabilities capabilities(wgpu::Default);

	surface.getCapabilities(adapter, &capabilities);

	core.RegisterResource(std::move(capabilities));
}

void CreateDevice(ES::Engine::Core &core) {
	ES::Utils::Log::Info("Creating WebGPU device...");

	const auto &adapter = core.GetResource<wgpu::Adapter>();

	if (adapter == nullptr) throw std::runtime_error("Adapter is not created, cannot create device.");

	wgpu::DeviceDescriptor deviceDesc(wgpu::Default);

	deviceDesc.label = wgpu::StringView("My Device");
	deviceDesc.requiredFeatureCount = 0;
	deviceDesc.requiredLimits = nullptr;
	deviceDesc.defaultQueue.nextInChain = nullptr;
	deviceDesc.defaultQueue.label = wgpu::StringView("The default queue");
	deviceDesc.deviceLostCallbackInfo = {};
	deviceDesc.deviceLostCallbackInfo.mode = wgpu::CallbackMode::AllowProcessEvents;
	deviceDesc.deviceLostCallbackInfo.callback = [](WGPUDevice const * device, WGPUDeviceLostReason reason, WGPUStringView message, WGPU_NULLABLE void* userdata1, WGPU_NULLABLE void* userdata2) {
		ES::Utils::Log::Error(fmt::format("Device lost: reason {:x} ({})", static_cast<uint32_t>(reason), toStdStringView(message)));
	};
	deviceDesc.uncapturedErrorCallbackInfo = {};
	deviceDesc.uncapturedErrorCallbackInfo.callback = [](WGPUDevice const * device, WGPUErrorType type, WGPUStringView message, WGPU_NULLABLE void* userdata1, WGPU_NULLABLE void* userdata2) {
		ES::Utils::Log::Error(fmt::format("Uncaptured device error: type {:x} ({})", static_cast<uint32_t>(type), toStdStringView(message)));
	};

	wgpu::Device device = core.RegisterResource(wgpu::Device(requestDeviceSync(adapter, deviceDesc)));

	if (device == nullptr) throw std::runtime_error("Could not create WebGPU device");

	ES::Utils::Log::Info(fmt::format("WebGPU device created: {}", static_cast<void*>(device)));
}

void CreateQueue(ES::Engine::Core &core) {
	ES::Utils::Log::Info("Creating WebGPU queue...");

	const auto &device = core.GetResource<wgpu::Device>();

	if (device == nullptr) throw std::runtime_error("WebGPU device is not created, cannot create queue.");

	wgpu::Queue &queue = core.RegisterResource(wgpu::Queue(wgpuDeviceGetQueue(device)));

	if (queue == nullptr) throw std::runtime_error("Could not create WebGPU queue");

	ES::Utils::Log::Info(fmt::format("WebGPU queue created: {}", static_cast<void*>(queue)));
}

void SetupQueueOnSubmittedWorkDone(ES::Engine::Core &core)
{
	auto &queue = core.GetResource<wgpu::Queue>();

	auto onQueueWorkDone = [](WGPUQueueWorkDoneStatus status, WGPU_NULLABLE void* userdata1, WGPU_NULLABLE void* userdata2) {
		ES::Utils::Log::Info(fmt::format("Queued work finished with status: {:x}", static_cast<uint32_t>(status)));
	};
	wgpu::QueueWorkDoneCallbackInfo callbackInfo(wgpu::Default);
	callbackInfo.callback = onQueueWorkDone;
	wgpuQueueOnSubmittedWorkDone(queue, callbackInfo);
}

void ConfigureSurface(ES::Engine::Core &core) {

	const auto &device = core.GetResource<wgpu::Device>();
	const auto &surface = core.GetResource<wgpu::Surface>();
	const auto &capabilities = core.GetResource<wgpu::SurfaceCapabilities>();
	auto &window = core.GetResource<ES::Plugin::Window::Resource::Window>();

	if (surface == nullptr) throw std::runtime_error("Surface is not created, cannot configure it.");
	if (device == nullptr) throw std::runtime_error("Device is not created, cannot configure surface.");

	int frameBufferSizeX, frameBufferSizeY;
	glfwGetFramebufferSize(window.GetGLFWWindow(), &frameBufferSizeX, &frameBufferSizeY);
	wgpu::SurfaceConfiguration config(wgpu::Default);
	config.width = frameBufferSizeX;
	config.height = frameBufferSizeY;
	config.usage = wgpu::TextureUsage::RenderAttachment;
	config.format = capabilities.formats[0];
	config.viewFormatCount = 0;
	config.viewFormats = nullptr;
	config.device = device;
	config.presentMode = wgpu::PresentMode::Fifo;
	config.alphaMode = wgpu::CompositeAlphaMode::Auto;

	surface.configure(config);
}

void ReleaseAdapter(ES::Engine::Core &core)
{
	auto &adapter = core.GetResource<wgpu::Adapter>();

	if (adapter == nullptr) throw std::runtime_error("WebGPU adapter is already released or was never created.");

	ES::Utils::Log::Info(fmt::format("Releasing WebGPU adapter: {}", static_cast<void*>(adapter)));
	adapter.release();
	adapter = nullptr;
	// TODO: Remove the adapter from the core resources (#252)
	ES::Utils::Log::Info("WebGPU adapter released.");
}

void CreateBindingGroup(ES::Engine::Core &core)
{
	auto &device = core.GetResource<wgpu::Device>();

	if (device == nullptr) throw std::runtime_error("WebGPU device is not created, cannot create binding group.");

	// Create a binding
	wgpu::BindGroupEntry binding(wgpu::Default);
	binding.binding = 0;
	binding.buffer = uniformBuffer;
	binding.size = sizeof(MyUniforms);

	// A bind group contains one or multiple bindings
	wgpu::BindGroupDescriptor bindGroupDesc(wgpu::Default);
	bindGroupDesc.layout = bindGroupLayout;
	bindGroupDesc.entryCount = 1;
	bindGroupDesc.entries = &binding;
	bindGroupDesc.label = wgpu::StringView("My Bind Group");
	const auto &bindGroup = core.RegisterResource(wgpu::BindGroup(device.createBindGroup(bindGroupDesc)));

	if (bindGroup == nullptr) throw std::runtime_error("Could not create WebGPU bind group");
}

wgpu::Texture textureToRelease = nullptr;

wgpu::TextureView GetNextSurfaceViewData(wgpu::Surface &surface){
	// Get the surface texture
	wgpu::SurfaceTexture surfaceTexture(wgpu::Default);
	surface.getCurrentTexture(&surfaceTexture);
	if (
	    surfaceTexture.status != wgpu::SurfaceGetCurrentTextureStatus::SuccessOptimal && // NEW
	    surfaceTexture.status != wgpu::SurfaceGetCurrentTextureStatus::SuccessSuboptimal
	) {
	    return nullptr;
	}
	wgpu::Texture texture = surfaceTexture.texture;

	// Create a view for this surface texture
	wgpu::TextureViewDescriptor viewDescriptor(wgpu::Default);
	viewDescriptor.label = wgpu::StringView("Surface texture view");
	viewDescriptor.format = texture.getFormat();
	viewDescriptor.dimension = wgpu::TextureViewDimension::_2D;
	viewDescriptor.mipLevelCount = 1;
	viewDescriptor.arrayLayerCount = 1;
	wgpu::TextureView targetView = texture.createView(viewDescriptor);

	textureToRelease = texture;

	return targetView;
}

wgpu::RenderPassEncoder renderPass = nullptr;
wgpu::TextureView textureView = nullptr;

void Clear(ES::Engine::Core &core) {
	wgpu::Device &device = core.GetResource<wgpu::Device>();
	wgpu::Surface &surface = core.GetResource<wgpu::Surface>();
	wgpu::RenderPipeline &pipeline = core.GetResource<wgpu::RenderPipeline>();
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

void DrawMesh(ES::Engine::Core &core, Mesh &mesh, ES::Plugin::Object::Component::Transform &transform) {
	wgpu::RenderPipeline &pipeline = core.GetResource<wgpu::RenderPipeline>();
	wgpu::Queue &queue = core.GetResource<wgpu::Queue>();
	wgpu::Device &device = core.GetResource<wgpu::Device>();

	if (textureView == nullptr) throw std::runtime_error("Texture view is not created, cannot draw mesh.");
	if (pipeline == nullptr) throw std::runtime_error("WebGPU render pipeline is not created, cannot draw mesh.");
	if (queue == nullptr) throw std::runtime_error("WebGPU queue is not created, cannot draw mesh.");
	if (device == nullptr) throw std::runtime_error("WebGPU device is not created, cannot draw mesh.");

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
	renderPass.setPipeline(pipeline);
	auto &bindGroup = core.GetResource<wgpu::BindGroup>();
	renderPass.setBindGroup(0, bindGroup, 0, nullptr);

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

	// std::cout << "Submitting command..." << std::endl;
	queue.submit(1, &command);
	command.release();
}

void DrawGui(ES::Engine::Core &core)
{
	wgpu::RenderPipeline &pipeline = core.GetResource<wgpu::RenderPipeline>();
	wgpu::Queue &queue = core.GetResource<wgpu::Queue>();
	wgpu::Device &device = core.GetResource<wgpu::Device>();

	if (textureView == nullptr) throw std::runtime_error("Texture view is not created, cannot draw mesh.");
	if (pipeline == nullptr) throw std::runtime_error("WebGPU render pipeline is not created, cannot draw mesh.");
	if (queue == nullptr) throw std::runtime_error("WebGPU queue is not created, cannot draw mesh.");
	if (device == nullptr) throw std::runtime_error("WebGPU device is not created, cannot draw mesh.");

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
	renderPass.setPipeline(pipeline);
	auto &bindGroup = core.GetResource<wgpu::BindGroup>();
	renderPass.setBindGroup(0, bindGroup, 0, nullptr);

	UpdateGui(renderPass, core);

	renderPass.end();
	renderPass.release();

	// Finally encode and submit the render pass
	wgpu::CommandBufferDescriptor cmdBufferDescriptor(wgpu::Default);
	cmdBufferDescriptor.label = wgpu::StringView("Command buffer");
	auto command = commandEncoder.finish(cmdBufferDescriptor);
	commandEncoder.release();

	// std::cout << "Submitting command..." << std::endl;
	queue.submit(1, &command);
	command.release();
}

void DrawMeshes(ES::Engine::Core &core)
{
	wgpu::Device &device = core.GetResource<wgpu::Device>();
	wgpu::Surface &surface = core.GetResource<wgpu::Surface>();
	wgpu::RenderPipeline &pipeline = core.GetResource<wgpu::RenderPipeline>();
	wgpu::Queue &queue = core.GetResource<wgpu::Queue>();

	if (device == nullptr) throw std::runtime_error("WebGPU device is not created, cannot draw.");
	if (surface == nullptr) throw std::runtime_error("WebGPU surface is not created, cannot draw.");
	if (pipeline == nullptr) throw std::runtime_error("WebGPU render pipeline is not created, cannot draw.");
	if (queue == nullptr) throw std::runtime_error("WebGPU queue is not created, cannot draw.");

	MyUniforms uniforms;

	uniforms.time = glfwGetTime();
	uniforms.color = { 0.0f, 1.0f, 0.4f, 1.0f };

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

	float angle1 = uniforms.time;
	glm::mat4x4 M(1.0);
	M = glm::rotate(M, angle1, glm::vec3(0.0, 0.0, 1.0));
	M = glm::translate(M, glm::vec3(0.5, 0.0, 0.0));
	M = glm::scale(M, glm::vec3(0.2f));
	uniforms.modelMatrix = M;
	queue.writeBuffer(uniformBuffer, offsetof(MyUniforms, time), &uniforms.time, sizeof(MyUniforms::time));
	queue.writeBuffer(uniformBuffer, offsetof(MyUniforms, color), &uniforms.color, sizeof(MyUniforms::color));
	queue.writeBuffer(uniformBuffer, offsetof(MyUniforms, modelMatrix), &uniforms.modelMatrix, sizeof(MyUniforms::modelMatrix));
	queue.writeBuffer(uniformBuffer, offsetof(MyUniforms, viewMatrix), &uniforms.viewMatrix, sizeof(MyUniforms::viewMatrix));
	queue.writeBuffer(uniformBuffer, offsetof(MyUniforms, projectionMatrix), &uniforms.projectionMatrix, sizeof(MyUniforms::projectionMatrix));


	core.GetRegistry().view<Mesh, ES::Plugin::Object::Component::Transform>().each([&](Mesh &mesh, ES::Plugin::Object::Component::Transform &transform) {
		if (!mesh.enabled) return;
		DrawMesh(core, mesh, transform);
	});

	DrawGui(core);

	// At the end of the frame
	textureView.release();
	surface.present();
}

void ReleaseBuffers(ES::Engine::Core &core)
{
	core.GetRegistry().view<Mesh>().each([](Mesh &mesh) {
		mesh.Release();
	});
}

void ReleaseDevice(ES::Engine::Core &core)
{
	wgpu::Device &device = core.GetResource<wgpu::Device>();

	if (device) {
		wgpuDeviceRelease(device);
		device = nullptr;
	}
}

void ReleaseSurface(ES::Engine::Core &core)
{
	wgpu::Surface &surface = core.GetResource<wgpu::Surface>();

	if (surface) {
		surface.unconfigure();
		surface.release();
		surface = nullptr;
	}
}

void ReleaseQueue(ES::Engine::Core &core)
{
	wgpu::Queue &queue = core.GetResource<wgpu::Queue>();

	if (queue) {
		queue.release();
		queue = nullptr;
	}
}

void ReleasePipeline(ES::Engine::Core &core)
{
	wgpu::RenderPipeline &pipeline = core.GetResource<wgpu::RenderPipeline>();

	if (pipeline) {
		pipeline.release();
		pipeline = nullptr;
	}
}

void ReleaseBindingGroup(ES::Engine::Core &core)
{
	wgpu::BindGroup &bindGroup = core.GetResource<wgpu::BindGroup>();

	if (bindGroup) {
		bindGroup.release();
		bindGroup = nullptr;
	}
}

void ReleaseUniforms(ES::Engine::Core &core)
{
	if (uniformBuffer) {
		uniformBuffer.release();
		uniformBuffer = nullptr;
	}
}


void terminateDepthBuffer(ES::Engine::Core &core) {
	if (depthTextureView) {
		depthTextureView.release();
		depthTextureView = nullptr;
	}
}

void initDepthBuffer(ES::Engine::Core &core) {
	auto &device = core.GetResource<wgpu::Device>();
	auto &window = core.GetResource<ES::Plugin::Window::Resource::Window>();
	if (device == nullptr) throw std::runtime_error("WebGPU device is not created, cannot initialize depth buffer.");

	int frameBufferSizeX, frameBufferSizeY;
	glfwGetFramebufferSize(window.GetGLFWWindow(), &frameBufferSizeX, &frameBufferSizeY);

	wgpu::TextureDescriptor depthTextureDesc(wgpu::Default);
	depthTextureDesc.label = wgpu::StringView("Z Buffer");
	depthTextureDesc.usage = wgpu::TextureUsage::RenderAttachment;
	depthTextureDesc.size = { static_cast<uint32_t>(frameBufferSizeX), static_cast<uint32_t>(frameBufferSizeY), 1u };
	depthTextureDesc.format = depthTextureFormat;
	wgpu::Texture depthTexture = device.createTexture(depthTextureDesc);

	depthTextureView = depthTexture.createView();
	depthTexture.release();
}

void unconfigureSurface(ES::Engine::Core &core) {
	auto &surface = core.GetResource<wgpu::Surface>();
	if (surface == nullptr) throw std::runtime_error("WebGPU surface is not created, cannot unconfigure it.");

	surface.unconfigure();
}

namespace ES::Plugin::WebGPU {
class Plugin : public ES::Engine::APlugin {
  public:
    using APlugin::APlugin;
    ~Plugin() = default;

    void Bind() final {
		RequirePlugins<ES::Plugin::Window::Plugin>();

		RegisterResource<ClearColor>(ClearColor());

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
			InitializePipeline,
			InitializeBuffers,
			CreateBindingGroup
		);
		RegisterSystems<ES::Plugin::RenderingPipeline::Draw>(
			Clear,
			DrawMeshes
		);
		RegisterSystems<ES::Engine::Scheduler::Shutdown>(
			ReleaseBindingGroup,
			ReleaseUniforms,
			ReleaseBuffers,
			ReleasePipeline,
			terminateDepthBuffer,
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

void MovementSystem(ES::Engine::Core &core)
{
	auto &cameraData = core.GetResource<CameraData>();

	glm::vec3 movementForce = GetKeyboardMovementForce(core);
	if (glm::length(movementForce) > 0.0f) {
		cameraData.position += movementForce * 1.f;
	}

}

glm::vec2 lastCursorPos(0.0f, 0.0f);

void onResize(GLFWwindow* window, int width, int height) {
	auto core = reinterpret_cast<ES::Engine::Core*>(glfwGetWindowUserPointer(window));

    if (core == nullptr) throw std::runtime_error("Window user pointer is null, cannot resize.");

	terminateDepthBuffer(*core);
	unconfigureSurface(*core);

	ConfigureSurface(*core);
	initDepthBuffer(*core);

	auto &cameraData = core->GetResource<CameraData>();
	cameraData.aspectRatio = static_cast<float>(width) / static_cast<float>(height);
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
		// Setup Platform/Renderer backends
		ImGui_ImplGlfw_InitForOther(window.GetGLFWWindow(), true);
		ImGui_ImplWGPU_InitInfo info = ImGui_ImplWGPU_InitInfo();
		info.DepthStencilFormat = depthTextureFormat;
		info.RenderTargetFormat = core.GetResource<wgpu::SurfaceCapabilities>().formats[0];
		info.Device = core.GetResource<wgpu::Device>();
		ImGui_ImplWGPU_Init(&info);
	});

	// TODO: avoid defining the camera data in the main.cpp, use default values
	core.RegisterResource<CameraData>({
		.position = { 0.0f, 300.0f, 0.0f },
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

	core.RegisterSystem(MovementSystem);

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
		core.GetResource<ES::Plugin::Window::Resource::Window>().SetResizable(true);
		core.GetResource<ES::Plugin::Window::Resource::Window>().SetFramebufferSizeCallback(&core, onResize);
		// Print pixel ratio
		auto &window = core.GetResource<ES::Plugin::Window::Resource::Window>();

		int displayX, displayY;
		glfwGetFramebufferSize(window.GetGLFWWindow(), &displayX, &displayY);
		int windowX, windowY;
		glfwGetWindowSize(window.GetGLFWWindow(), &windowX, &windowY);
		std::cout << "Display size: " << displayX << "x" << displayY << std::endl;
		std::cout << "Window size: " << windowX << "x" << windowY << std::endl;
		std::cout << "Pixel ratio: " << static_cast<float>(displayX) / static_cast<float>(windowX) << std::endl;
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

		inputManager.RegisterKeyCallback([](ES::Engine::Core &cbCore, int key, int, int action, int) {
			if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
				cbCore.Stop();
			}

			ImGuiIO& io = ImGui::GetIO();
			if (io.WantCaptureMouse) {
				// Don't rotate the camera if the mouse is already captured by an ImGui
				// interaction at this frame.
				return;
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
	});

	core.RunCore();

	return 0;
}
