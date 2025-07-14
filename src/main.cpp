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

struct MyUniforms {
	glm::mat4x4 projectionMatrix;
    glm::mat4x4 viewMatrix;
    glm::mat4x4 modelMatrix;
    glm::vec4 color;
    float time;
    float _pad[3];
};

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

// This assert should stay here as we want this rule to link struct to webgpu struct
static_assert(sizeof(MyUniforms) % 16 == 0);

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

struct Mesh {
	wgpu::Buffer pointBuffer = nullptr;
	wgpu::Buffer indexBuffer = nullptr;
	uint32_t indexCount = 0;

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


		std::vector<uint16_t> indexData;
		for (size_t i = 0; i < indices.size(); i++) {
			indexData.push_back(static_cast<uint16_t>(indices.at(i)));
		}

		indexCount = static_cast<uint32_t>(indexData.size());

		wgpu::BufferDescriptor bufferDesc(wgpu::Default);
		bufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex;
		bufferDesc.mappedAtCreation = false;
		bufferDesc.size = pointData.size() * sizeof(float);
		bufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex; // Vertex usage here!
		pointBuffer = device.createBuffer(bufferDesc);

		queue.writeBuffer(pointBuffer, 0, pointData.data(), bufferDesc.size);

		bufferDesc.size = indexData.size() * sizeof(uint16_t);
		indexData.resize((indexData.size() + 1) & ~1);
		bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index;
		indexBuffer = device.createBuffer(bufferDesc);

		queue.writeBuffer(indexBuffer, 0, indexData.data(), bufferDesc.size);
	}
};

// TODO: Release them
Mesh mesh;
wgpu::Buffer uniformBuffer = nullptr;
wgpu::PipelineLayout layout = nullptr;
wgpu::BindGroupLayout bindGroupLayout = nullptr;
wgpu::BindGroup bindGroup = nullptr;
wgpu::TextureFormat depthTextureFormat = wgpu::TextureFormat::Depth24Plus;
wgpu::TextureView depthTextureView = nullptr;

void InitializeBuffers(ES::Engine::Core &core)
{
	wgpu::Queue &queue = core.GetResource<wgpu::Queue>();
	wgpu::Device &device = core.GetResource<wgpu::Device>();

	if (queue == nullptr) throw std::runtime_error("WebGPU queue is not created, cannot initialize buffers.");
	if (device == nullptr) throw std::runtime_error("WebGPU device is not created, cannot initialize buffers.");

	std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;
    std::vector<uint32_t> indices;

    bool success = ES::Plugin::Object::Resource::OBJLoader::loadModel("assets/finish.obj", vertices, normals, texCoords, indices);
	if (!success) throw std::runtime_error("Model cant be loaded");

	mesh = Mesh(core, vertices, normals, indices);

	wgpu::BufferDescriptor bufferDesc(wgpu::Default);
	bufferDesc.size = sizeof(MyUniforms);
	bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
	uniformBuffer = device.createBuffer(bufferDesc);

	// Upload the initial value of the uniforms
	MyUniforms uniforms;
	uniforms.time = 1.0f;
	uniforms.color = { 0.0f, 1.0f, 0.4f, 1.0f };
	float near = 0.001f;
	float far = 100.0f;
	float ratio = 800.0f / 800.0f;
	float fov = glm::radians(45.0f);
	uniforms.projectionMatrix = glm::perspective(fov, ratio, near, far);
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

	wgpu::TextureDescriptor depthTextureDesc(wgpu::Default);
	depthTextureDesc.label = wgpu::StringView("Z Buffer");
	depthTextureDesc.usage = wgpu::TextureUsage::RenderAttachment;
	depthTextureDesc.size = { 800, 800, 1 };
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

	if (surface == nullptr) throw std::runtime_error("Surface is not created, cannot configure it.");
	if (device == nullptr) throw std::runtime_error("Device is not created, cannot configure surface.");

	wgpu::SurfaceConfiguration config(wgpu::Default);
	config.width = 800;
	config.height = 800;
	config.usage = wgpu::TextureUsage::RenderAttachment;
	config.format = capabilities.formats[0];
	config.viewFormatCount = 0;
	config.viewFormats = nullptr;
	config.device = device;
	config.presentMode = wgpu::PresentMode::Fifo;
	config.alphaMode = wgpu::CompositeAlphaMode::Auto;

	wgpuSurfaceConfigure(surface, &config);
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
	bindGroup = device.createBindGroup(bindGroupDesc);

	if (bindGroup == nullptr) throw std::runtime_error("Could not create WebGPU bind group");
}

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

	// texture.release();

	return targetView;
}

wgpu::RenderPassEncoder renderPass = nullptr;
wgpu::TextureView textureView = nullptr;

struct ClearColor {
	wgpu::Color value = { 0.05, 0.05, 0.05, 1.0 };
};

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

void DrawWebGPU(ES::Engine::Core &core)
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

	if (!textureView) return;

	// Create a command encoder for the draw call
	wgpu::CommandEncoderDescriptor encoderDesc(wgpu::Default);
	encoderDesc.label = wgpu::StringView("My command encoder");
	auto encoder = device.createCommandEncoder(encoderDesc);

	// Create the render pass that clears the screen with our color
	wgpu::RenderPassDescriptor renderPassDesc(wgpu::Default);
	renderPassDesc.label = wgpu::StringView("My render pass");

	// The attachment part of the render pass descriptor describes the target texture of the pass
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

	wgpu::RenderPassEncoder renderPass = encoder.beginRenderPass(renderPassDesc);

		// Select which render pipeline to use
	renderPass.setPipeline(pipeline);
	renderPass.setBindGroup(0, bindGroup, 0, nullptr);

	renderPass.setVertexBuffer(0, mesh.pointBuffer, 0, mesh.pointBuffer.getSize());
	renderPass.setIndexBuffer(mesh.indexBuffer, wgpu::IndexFormat::Uint16, 0, mesh.indexBuffer.getSize());

	// Set binding group
	renderPass.drawIndexed(mesh.indexCount, 1, 0, 0, 0);

	renderPass.end();
	// renderPass.release();

	// Finally encode and submit the render pass
	wgpu::CommandBufferDescriptor cmdBufferDescriptor(wgpu::Default);
	cmdBufferDescriptor.label = wgpu::StringView("Command buffer");
	auto command = encoder.finish(cmdBufferDescriptor);
	// encoder.release();

	// std::cout << "Submitting command..." << std::endl;
	queue.submit(1, &command);
	// command.release();
	// std::cout << "Command submitted." << std::endl;

	// At the end of the frame
	// targetView.release();
	textureView.release();
	surface.present();
}

void ReleaseBuffers(ES::Engine::Core &core)
{
	if (mesh.indexBuffer) {
		mesh.indexBuffer.release();
		mesh.indexBuffer = nullptr;
	}

	if (mesh.pointBuffer) {
		mesh.pointBuffer.release();
		mesh.pointBuffer = nullptr;
	}

	if (uniformBuffer) {
		uniformBuffer.release();
		uniformBuffer = nullptr;
	}
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
			DrawWebGPU
		);
		RegisterSystems<ES::Engine::Scheduler::Shutdown>(
			ReleaseBuffers,
			ReleaseDevice,
			ReleaseSurface,
			ReleaseQueue,
			ReleasePipeline
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
		cameraData.position += movementForce * 0.04f;
	}

}

glm::vec2 lastCursorPos(0.0f, 0.0f);

auto main(int ac, char **av) -> int
{
	ES::Engine::Core core;

	core.AddPlugins<ES::Plugin::WebGPU::Plugin, ES::Plugin::Input::Plugin>();

	core.RegisterResource<CameraData>({
		.position = { 0.0f, 3.0f, 3.0f },
		.yaw = 4.75f,
		.pitch = -0.75f,
		.up = { 0.0f, 1.0f, 0.0f },
		.fovY = glm::radians(45.0f),
		.nearPlane = 0.001f,
		.farPlane = 100.0f,
		.aspectRatio = 800.0f / 800.0f
	});

	core.RegisterSystem(MovementSystem);
	core.RegisterSystem([](ES::Engine::Core &core) {
		auto &cameraData = core.GetResource<CameraData>();
	});

	core.RegisterSystem([](ES::Engine::Core &core) {
		auto &clearColor = core.GetResource<ClearColor>();
		auto dt = core.GetScheduler<ES::Engine::Scheduler::Update>().GetDeltaTime();
		static float time = 0.0f;
		time += dt;

		clearColor.value.r = 0.5f + 0.5f * std::sin(time * 0.5f);
		clearColor.value.g = 0.5f + 0.5f * std::sin(time * 0.7f);
		clearColor.value.b = 0.5f + 0.5f * std::sin(time * 0.9f);
	});

	core.RegisterSystem<ES::Engine::Scheduler::Startup>([&](ES::Engine::Core &core) {
		// lock cursor position to the center of the window
		auto &window = core.GetResource<ES::Plugin::Window::Resource::Window>();
		glfwSetInputMode(window.GetGLFWWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);



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
		});

		inputManager.RegisterCursorPosCallback([&](ES::Engine::Core &, double x, double y) {
			auto &cameraData = core.GetResource<CameraData>();
			auto &window = core.GetResource<ES::Plugin::Window::Resource::Window>();
			glm::vec2 windowSize = window.GetSize();

			if (lastCursorPos.x == 0.0f && lastCursorPos.y == 0.0f) {
				lastCursorPos = { static_cast<float>(x), static_cast<float>(y) };
				return; // Skip the first call to avoid jump
			}

			glm::vec2 cursorOffset = { x - lastCursorPos.x, y - lastCursorPos.y };
			lastCursorPos = { x, y };

			static float sensitivity = 0.003f;
			cameraData.yaw += sensitivity * -cursorOffset.x;
			cameraData.pitch += sensitivity * -cursorOffset.y;
			cameraData.pitch = glm::clamp(cameraData.pitch, glm::radians(-89.0f), glm::radians(89.0f));
		});
	});

	core.RunCore();

	return 0;
}
