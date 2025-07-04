#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#include <glm/ext.hpp>

#include <webgpu/webgpu.h>
#include <glfw3webgpu.h>
#include <iostream>
#include <vector>
#include <fstream>
#include "Engine.hpp"
#include "PluginWindow.hpp"
#include "Window.hpp"
#include "Object.hpp"
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <GLFW/glfw3.h>

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

uint32_t ceilToNextMultiple(uint32_t value, uint32_t step) {
    uint32_t divide_and_ceil = value / step + (value % step == 0 ? 0 : 1);
    return step * divide_and_ceil;
}

std::string_view toStdStringView(WGPUStringView wgpuStringView) {
    return
        wgpuStringView.data == nullptr
        ? std::string_view()
        : wgpuStringView.length == WGPU_STRLEN
        ? std::string_view(wgpuStringView.data)
        : std::string_view(wgpuStringView.data, wgpuStringView.length);
}

WGPUStringView toWgpuStringView(std::string_view stdStringView) {
    return { stdStringView.data(), stdStringView.size() };
}
WGPUStringView toWgpuStringView(const char* cString) {
    return { cString, WGPU_STRLEN };
}

auto requestAdapterSync(WGPUInstance instance, WGPURequestAdapterOptions const * options)
	-> WGPUAdapter
{
    struct UserData {
        WGPUAdapter adapter = nullptr;
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
        if (status == WGPURequestAdapterStatus_Success) {
			userData.adapter = adapter;
		} else {
			userData.adapter = {0};
			ES::Utils::Log::Error(fmt::format("Could not get WebGPU adapter: {}", toStdStringView(message)));
		}
    };

	// TODO: Use proper struct initialization
    WGPURequestAdapterCallbackInfo callbackInfo = {};
    callbackInfo.callback = onAdapterRequestEnded;
    callbackInfo.userdata1 = &userData;

	wgpuInstanceRequestAdapter(instance, nullptr, callbackInfo);

    while (!userData.requestEnded) {
		wgpuInstanceProcessEvents(instance);
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}

    return userData.adapter;
}

void AdaptaterPrintLimits(ES::Engine::Core &core)
{
	const WGPUAdapter &adapter = core.GetResource<WGPUAdapter>();
	if (adapter == nullptr) throw std::runtime_error("Adapter is null, cannot print limits.");

	// TODO: Use proper struct initialization
	WGPULimits supportedLimits = {0};

	WGPUStatus success = wgpuAdapterGetLimits(adapter, &supportedLimits);

	if (success != WGPUStatus_Success) throw std::runtime_error("Failed to get adapter limits");

	ES::Utils::Log::Info("Adapter limits:");
	ES::Utils::Log::Info(fmt::format(" - maxTextureDimension1D: {}", supportedLimits.maxTextureDimension1D));
	ES::Utils::Log::Info(fmt::format(" - maxTextureDimension2D: {}", supportedLimits.maxTextureDimension2D));
	ES::Utils::Log::Info(fmt::format(" - maxTextureDimension3D: {}", supportedLimits.maxTextureDimension3D));
	ES::Utils::Log::Info(fmt::format(" - maxTextureArrayLayers: {}", supportedLimits.maxTextureArrayLayers));
}

void AdaptaterPrintFeatures(ES::Engine::Core &core)
{
	const WGPUAdapter &adapter = core.GetResource<WGPUAdapter>();
	if (adapter == nullptr) throw std::runtime_error("Adapter is null, cannot print features.");

	// TODO: Use proper struct initialization
	WGPUSupportedFeatures features = {0};

	wgpuAdapterGetFeatures(adapter, &features);

	ES::Utils::Log::Info("Adapter features:");
	for (size_t i = 0; i < features.featureCount; ++i) {
		ES::Utils::Log::Info(fmt::format(" - 0x{:x}", static_cast<uint32_t>(features.features[i])));
	}

	wgpuSupportedFeaturesFreeMembers(features);
}

void AdaptaterPrintProperties(ES::Engine::Core &core)
{
	const WGPUAdapter &adapter = core.GetResource<WGPUAdapter>();
	if (adapter == nullptr) throw std::runtime_error("Adapter is null, cannot print properties.");

	// TODO: Use proper struct initialization
	WGPUAdapterInfo properties = {0};

	wgpuAdapterGetInfo(adapter, &properties);

	ES::Utils::Log::Info("Adapter properties:");
	ES::Utils::Log::Info(fmt::format(" - vendorID: {}", properties.vendorID));
	ES::Utils::Log::Info(fmt::format(" - vendorName: {}", toStdStringView(properties.vendor)));
	ES::Utils::Log::Info(fmt::format(" - architecture: {}", toStdStringView(properties.architecture)));
	ES::Utils::Log::Info(fmt::format(" - deviceID: {}", properties.deviceID));
	ES::Utils::Log::Info(fmt::format(" - name: {}", toStdStringView(properties.device)));
	ES::Utils::Log::Info(fmt::format(" - driverDescription: {}", toStdStringView(properties.description)));
	ES::Utils::Log::Info(fmt::format(" - adapterType: 0x{:x}", static_cast<uint32_t>(properties.adapterType)));
	ES::Utils::Log::Info(fmt::format(" - backendType: 0x{:x}", static_cast<uint32_t>(properties.backendType)));

	wgpuAdapterInfoFreeMembers(properties);
}

WGPUDevice requestDeviceSync(const WGPUAdapter &adapter, WGPUDeviceDescriptor const * descriptor) {
    struct UserData {
        WGPUDevice device = nullptr;
        bool requestEnded = false;
    };
    UserData userData;

    auto onDeviceRequestEnded = []
	(WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message, WGPU_NULLABLE void* userdata1, WGPU_NULLABLE void* userdata2) {
        UserData& userData = *reinterpret_cast<UserData*>(userdata1);
        if (status == WGPURequestDeviceStatus_Success) {
            userData.device = device;
        } else {
			userData.device = nullptr;
			ES::Utils::Log::Error(fmt::format("Could not get WebGPU device: {}", toStdStringView(message)));
		}
        userData.requestEnded = true;
    };

	// TODO: Use proper struct initialization
	WGPURequestDeviceCallbackInfo callbackInfo = {};
    callbackInfo.callback = onDeviceRequestEnded;
    callbackInfo.userdata1 = &userData;

    wgpuAdapterRequestDevice(
        adapter,
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
    const WGPUDevice &device = core.GetResource<WGPUDevice>();
    if (device == nullptr) throw std::runtime_error("Device is not created, cannot inspect it.");

    WGPUSupportedFeatures features = {};
    wgpuDeviceGetFeatures(device, &features);

    ES::Utils::Log::Info("Device features:");
    for (size_t i = 0; i < features.featureCount; ++i) {
        ES::Utils::Log::Info(fmt::format(" - 0x{:x}", static_cast<uint32_t>(features.features[i])));
    }
    wgpuSupportedFeaturesFreeMembers(features);

    WGPULimits limits = {0};
    limits.nextInChain = nullptr;
	limits.maxUniformBufferBindingSize = 16 * 4 * sizeof(float);

    bool success = wgpuDeviceGetLimits(device, &limits) == WGPUStatus_Success;

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
WGPUBuffer indexBuffer = nullptr;
WGPUBuffer pointBuffer = nullptr;
WGPUBuffer uniformBuffer = nullptr;
WGPUPipelineLayout layout = nullptr;
WGPUBindGroupLayout bindGroupLayout = nullptr;
uint32_t indexCount = 0;
WGPUBindGroup bindGroup = nullptr;
WGPUTextureFormat depthTextureFormat = WGPUTextureFormat_Depth24Plus;
WGPUTextureView depthTextureView = nullptr;

void InitializeBuffers(ES::Engine::Core &core)
{
	WGPUQueue &queue = core.GetResource<WGPUQueue>();
	WGPUDevice &device = core.GetResource<WGPUDevice>();

	if (queue == nullptr) throw std::runtime_error("WebGPU queue is not created, cannot initialize buffers.");
	if (device == nullptr) throw std::runtime_error("WebGPU device is not created, cannot initialize buffers.");

	std::vector<float> pointData;
	std::vector<uint16_t> indexData;

	std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;
    std::vector<uint32_t> indices;

    bool success = ES::Plugin::Object::Resource::OBJLoader::loadModel("assets/finish.obj", vertices, normals, texCoords, indices);
	if (!success) throw std::runtime_error("Model cant be loaded");

	for (size_t i = 0; i < vertices.size(); i++) {
		pointData.push_back(vertices.at(i).x);
		pointData.push_back(vertices.at(i).y);
		pointData.push_back(vertices.at(i).z);
		pointData.push_back(normals.at(i).r);
		pointData.push_back(normals.at(i).g);
		pointData.push_back(normals.at(i).b);
	}

	for (size_t i = 0; i < indices.size(); i++) {
		indexData.push_back(static_cast<uint16_t>(indices.at(i)));
	}

	indexCount = static_cast<uint32_t>(indexData.size());

	WGPUBufferDescriptor bufferDesc = {
		NULL,
		toWgpuStringView(NULL),
		WGPUBufferUsage_None,
		0,
		false
	};
	bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
	bufferDesc.mappedAtCreation = false;
	bufferDesc.size = pointData.size() * sizeof(float);
	bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex; // Vertex usage here!
	pointBuffer = wgpuDeviceCreateBuffer(device, &bufferDesc);

	wgpuQueueWriteBuffer(queue, pointBuffer, 0, pointData.data(), bufferDesc.size);

	bufferDesc.size = indexData.size() * sizeof(uint16_t);
	bufferDesc.size = (bufferDesc.size + 3) & ~3;
	indexData.resize((indexData.size() + 1) & ~1);
	bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index;
	indexBuffer = wgpuDeviceCreateBuffer(device, &bufferDesc);

	wgpuQueueWriteBuffer(queue, indexBuffer, 0, indexData.data(), bufferDesc.size);

	bufferDesc.size = sizeof(MyUniforms);
	bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
	uniformBuffer = wgpuDeviceCreateBuffer(device, &bufferDesc);

	// Upload the initial value of the uniforms
	MyUniforms uniforms;
	uniforms.time = 1.0f;
	uniforms.color = { 0.0f, 1.0f, 0.4f, 1.0f };
	float near = 0.001f;
	float far = 100.0f;
	float ratio = 800.0f / 800.0f;
	float fov = glm::radians(45.0f);
	uniforms.projectionMatrix = glm::perspective(fov, ratio, near, far);
	wgpuQueueWriteBuffer(queue, uniformBuffer, 0, &uniforms, sizeof(uniforms));
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
	WGPUDevice &device = core.GetResource<WGPUDevice>();
	WGPUTextureFormat surfaceFormat = core.GetResource<WGPUSurfaceCapabilities>().formats[0];
	// In Initialize() or in a dedicated InitializePipeline()
    WGPUShaderSourceWGSL wgslDesc = {
		{
			NULL,
			WGPUSType_ShaderSourceWGSL
		},
		toWgpuStringView(NULL)
	};
	// Load the shader source from a file or a string
	std::string shaderSource = loadFile("shader.wgsl");
    wgslDesc.code = toWgpuStringView(shaderSource);
    WGPUShaderModuleDescriptor shaderDesc = {
		NULL,
		toWgpuStringView("My Shader Module")
	};
    shaderDesc.nextInChain = &wgslDesc.chain; // connect the chained extension
    shaderDesc.label = toWgpuStringView("Shader source from Application.cpp");
    WGPUShaderModule shaderModule = wgpuDeviceCreateShaderModule(device, &shaderDesc);
    WGPURenderPipelineDescriptor pipelineDesc = {
		NULL,
		toWgpuStringView("My Render Pipeline"),
		NULL,
		{
			NULL,
			NULL,
			toWgpuStringView(NULL),
			0,
			NULL,
			0,
			NULL
		},
		{
			NULL,
			WGPUPrimitiveTopology_Undefined,
			(WGPUIndexFormat)0,
			WGPUFrontFace_Undefined,
			WGPUCullMode_Undefined,
			false
		},
		NULL,
		{
			NULL,
			1,
			0xFFFFFFFF,
			false
		},
		NULL
	};
    WGPUVertexBufferLayout vertexBufferLayout = {
		WGPUVertexStepMode_Undefined,
		0,
		0,
		NULL
	};

	std::vector<WGPUVertexAttribute> vertexAttribs(2);

    // Describe the position attribute
    vertexAttribs[0].shaderLocation = 0;
    vertexAttribs[0].format = WGPUVertexFormat_Float32x3;
    vertexAttribs[0].offset = 0;

    // Describe the color attribute
    vertexAttribs[1].shaderLocation = 1;
    vertexAttribs[1].format = WGPUVertexFormat_Float32x3;
    vertexAttribs[1].offset = 3 * sizeof(float);

    vertexBufferLayout.attributeCount = static_cast<uint32_t>(vertexAttribs.size());
    vertexBufferLayout.attributes = vertexAttribs.data();

    vertexBufferLayout.arrayStride = 6 * sizeof(float);
    vertexBufferLayout.stepMode = WGPUVertexStepMode_Vertex;

	// Define binding layout
	WGPUBindGroupLayoutEntry bindingLayout = {0};
	bindingLayout.binding = 0;
	bindingLayout.visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
	bindingLayout.buffer.type = WGPUBufferBindingType_Uniform;
	bindingLayout.buffer.minBindingSize = sizeof(MyUniforms);

	// Create a bind group layout
	WGPUBindGroupLayoutDescriptor bindGroupLayoutDesc = {0};
	bindGroupLayoutDesc.nextInChain = nullptr;
	bindGroupLayoutDesc.entryCount = 1;
	bindGroupLayoutDesc.entries = &bindingLayout;
	bindGroupLayoutDesc.label = toWgpuStringView("My Bind Group Layout");
	bindGroupLayout = wgpuDeviceCreateBindGroupLayout(device, &bindGroupLayoutDesc);

	// Create the pipeline layout
	WGPUPipelineLayoutDescriptor layoutDesc = {0};
	layoutDesc.nextInChain = nullptr;
	layoutDesc.bindGroupLayoutCount = 1;
	layoutDesc.bindGroupLayouts = &bindGroupLayout;
	layout = wgpuDeviceCreatePipelineLayout(device, &layoutDesc);

    // When describing the render pipeline:
    pipelineDesc.vertex.bufferCount = 1;
    pipelineDesc.vertex.buffers = &vertexBufferLayout;
	pipelineDesc.vertex.module = shaderModule;
    pipelineDesc.vertex.entryPoint = toWgpuStringView("vs_main");
    WGPUFragmentState fragmentState = {
		NULL,
		NULL,
		toWgpuStringView(NULL),
		0,
		NULL,
		0,
		NULL
	};
    fragmentState.module = shaderModule;
    fragmentState.entryPoint = toWgpuStringView("fs_main");
    WGPUColorTargetState colorTarget = {
		NULL,
		WGPUTextureFormat_Undefined,
		NULL,
		WGPUColorWriteMask_All
	};
    colorTarget.format = surfaceFormat;
    WGPUBlendState blendState = {
		{
			WGPUBlendOperation_Undefined,
			WGPUBlendFactor_Undefined,
			WGPUBlendFactor_Undefined
		},
		{
			WGPUBlendOperation_Undefined,
			WGPUBlendFactor_Undefined,
			WGPUBlendFactor_Undefined
		}
	};
    colorTarget.blend = &blendState;
    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTarget;
    pipelineDesc.fragment = &fragmentState;
	pipelineDesc.label = toWgpuStringView("My Render Pipeline");
	pipelineDesc.layout = layout;

	WGPUTextureDescriptor depthTextureDesc = {0};
	depthTextureDesc.label = toWgpuStringView("Z Buffer");
	depthTextureDesc.usage = WGPUTextureUsage_RenderAttachment;
	depthTextureDesc.size = { 800, 800, 1 };
	depthTextureDesc.format = depthTextureFormat;
	depthTextureDesc.mipLevelCount = 1;
	depthTextureDesc.sampleCount = 1;
	WGPUTexture depthTexture = wgpuDeviceCreateTexture(device, &depthTextureDesc);

	// Create the view of the depth texture manipulated by the rasterizer
	depthTextureView = wgpuTextureCreateView(depthTexture, nullptr);

	// We can now release the texture and only hold to the view
	wgpuTextureRelease(depthTexture);

	WGPUDepthStencilState depthStencilState = {0};
	depthStencilState.depthCompare = WGPUCompareFunction_Less;
	depthStencilState.depthWriteEnabled = WGPUOptionalBool_True;
	depthStencilState.format = depthTextureFormat;
	pipelineDesc.depthStencil = &depthStencilState;

	WGPURenderPipeline &pipeline = core.RegisterResource(wgpuDeviceCreateRenderPipeline(device, &pipelineDesc));

	if (pipeline == nullptr) throw std::runtime_error("Could not create render pipeline");

	wgpuShaderModuleRelease(shaderModule);
}

void CreateSurface(ES::Engine::Core &core) {
	ES::Utils::Log::Info("Creating surface...");

	auto &instance = core.GetResource<WGPUInstance>();
	auto glfwWindow = core.GetResource<ES::Plugin::Window::Resource::Window>().GetGLFWWindow();

	WGPUSurface &surface = core.RegisterResource(glfwCreateWindowWGPUSurface(instance, glfwWindow));

	if (surface == nullptr) throw std::runtime_error("Could not create surface");

	ES::Utils::Log::Info(fmt::format("Surface created: {}", static_cast<void*>(surface)));
}

void CreateInstance(ES::Engine::Core &core) {
	ES::Utils::Log::Info("Creating WebGPU instance...");

	WGPUInstanceDescriptor desc = {0};

	WGPUInstance instance = core.RegisterResource(wgpuCreateInstance(&desc));

	if (instance == nullptr) throw std::runtime_error("Could not create WebGPU instance");

	ES::Utils::Log::Info(fmt::format("WebGPU instance created: {}", static_cast<void*>(instance)));
}

void CreateAdapter(ES::Engine::Core &core) {
	ES::Utils::Log::Info("Requesting adapter...");

	auto &instance = core.GetResource<WGPUInstance>();
	auto &surface = core.GetResource<WGPUSurface>();

	if (instance == nullptr) throw std::runtime_error("WebGPU instance is not created, cannot request adapter");
	if (surface == nullptr) throw std::runtime_error("Surface is not created, cannot request adapter.");

	WGPURequestAdapterOptions adapterOpts = {0};
	adapterOpts.compatibleSurface = surface;

	WGPUAdapter adapter = core.RegisterResource(requestAdapterSync(instance, &adapterOpts));

	if (adapter == nullptr) throw std::runtime_error("Could not get WebGPU adapter");

	ES::Utils::Log::Info(fmt::format("Got adapter: {}", static_cast<void*>(adapter)));
}

void ReleaseInstance(ES::Engine::Core &core) {
	WGPUInstance &instance = core.GetResource<WGPUInstance>();

	if (instance == nullptr) throw std::runtime_error("WebGPU instance is already released or was never created.");

	ES::Utils::Log::Info(fmt::format("Releasing WebGPU instance: {}", static_cast<void*>(instance)));
	wgpuInstanceRelease(instance);
	instance = nullptr;
	// TODO: Remove the instance from the core resources (#252)
}

void RequestCapabilities(ES::Engine::Core &core)
{
	const WGPUAdapter &adapter = core.GetResource<WGPUAdapter>();
	const WGPUSurface &surface = core.GetResource<WGPUSurface>();

	if (adapter == nullptr) throw std::runtime_error("Adapter is not created, cannot request capabilities.");
	if (surface == nullptr) throw std::runtime_error("Surface is not created, cannot request capabilities.");

	WGPUSurfaceCapabilities capabilities = {0};

	wgpuSurfaceGetCapabilities(surface, adapter, &capabilities);

	core.RegisterResource(std::move(capabilities));
}

void CreateDevice(ES::Engine::Core &core) {
	ES::Utils::Log::Info("Creating WebGPU device...");

	const WGPUAdapter &adapter = core.GetResource<WGPUAdapter>();

	if (adapter == nullptr) throw std::runtime_error("Adapter is not created, cannot create device.");

	WGPUDeviceDescriptor deviceDesc = {};

	deviceDesc.nextInChain = nullptr;
	deviceDesc.label = toWgpuStringView("My Device");
	deviceDesc.requiredFeatureCount = 0;
	deviceDesc.requiredLimits = nullptr;
	deviceDesc.defaultQueue.nextInChain = nullptr;
	deviceDesc.defaultQueue.label = toWgpuStringView("The default queue");
	deviceDesc.deviceLostCallbackInfo = {};
	deviceDesc.deviceLostCallbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;
	deviceDesc.deviceLostCallbackInfo.callback = [](WGPUDevice const * device, WGPUDeviceLostReason reason, WGPUStringView message, WGPU_NULLABLE void* userdata1, WGPU_NULLABLE void* userdata2) {
		ES::Utils::Log::Error(fmt::format("Device lost: reason {:x} ({})", static_cast<uint32_t>(reason), toStdStringView(message)));
	};
	deviceDesc.uncapturedErrorCallbackInfo = {};
	deviceDesc.uncapturedErrorCallbackInfo.callback = [](WGPUDevice const * device, WGPUErrorType type, WGPUStringView message, WGPU_NULLABLE void* userdata1, WGPU_NULLABLE void* userdata2) {
		ES::Utils::Log::Error(fmt::format("Uncaptured device error: type {:x} ({})", static_cast<uint32_t>(type), toStdStringView(message)));
	};

	WGPUDevice device = core.RegisterResource(requestDeviceSync(adapter, &deviceDesc));

	if (device == nullptr) throw std::runtime_error("Could not create WebGPU device");

	ES::Utils::Log::Info(fmt::format("WebGPU device created: {}", static_cast<void*>(device)));
}

void CreateQueue(ES::Engine::Core &core) {
	ES::Utils::Log::Info("Creating WebGPU queue...");

	const WGPUDevice &device = core.GetResource<WGPUDevice>();

	if (device == nullptr) throw std::runtime_error("WebGPU device is not created, cannot create queue.");

	WGPUQueue &queue = core.RegisterResource(wgpuDeviceGetQueue(device));

	if (queue == nullptr) throw std::runtime_error("Could not create WebGPU queue");

	ES::Utils::Log::Info(fmt::format("WebGPU queue created: {}", static_cast<void*>(queue)));
}

void SetupQueueOnSubmittedWorkDone(ES::Engine::Core &core)
{
	WGPUQueue &queue = core.GetResource<WGPUQueue>();

	auto onQueueWorkDone = [](WGPUQueueWorkDoneStatus status, WGPU_NULLABLE void* userdata1, WGPU_NULLABLE void* userdata2) {
		ES::Utils::Log::Info(fmt::format("Queued work finished with status: {:x}", static_cast<uint32_t>(status)));
	};
	WGPUQueueWorkDoneCallbackInfo callbackInfo = {0};
	callbackInfo.callback = onQueueWorkDone;
	wgpuQueueOnSubmittedWorkDone(queue, callbackInfo);
}

void ConfigureSurface(ES::Engine::Core &core) {

	const WGPUDevice &device = core.GetResource<WGPUDevice>();
	const WGPUSurface &surface = core.GetResource<WGPUSurface>();
	const WGPUSurfaceCapabilities &capabilities = core.GetResource<WGPUSurfaceCapabilities>();

	if (surface == nullptr) throw std::runtime_error("Surface is not created, cannot configure it.");
	if (device == nullptr) throw std::runtime_error("Device is not created, cannot configure surface.");

	WGPUSurfaceConfiguration config = {};
	config.nextInChain = nullptr;
	config.width = 800;
	config.height = 800;
	config.usage = WGPUTextureUsage_RenderAttachment;
	config.format = capabilities.formats[0];
	config.viewFormatCount = 0;
	config.viewFormats = nullptr;
	config.device = device;
	config.presentMode = WGPUPresentMode_Fifo;
	config.alphaMode = WGPUCompositeAlphaMode_Auto;

	wgpuSurfaceConfigure(surface, &config);
}

void ReleaseAdapter(ES::Engine::Core &core)
{
	auto &adapter = core.GetResource<WGPUAdapter>();

	if (adapter == nullptr) throw std::runtime_error("WebGPU adapter is already released or was never created.");

	ES::Utils::Log::Info(fmt::format("Releasing WebGPU adapter: {}", static_cast<void*>(adapter)));
	wgpuAdapterRelease(adapter);
	adapter = nullptr;
	// TODO: Remove the adapter from the core resources (#252)
	ES::Utils::Log::Info("WebGPU adapter released.");
}

void CreateBindingGroup(ES::Engine::Core &core)
{
	WGPUDevice &device = core.GetResource<WGPUDevice>();

	if (device == nullptr) throw std::runtime_error("WebGPU device is not created, cannot create binding group.");

	// Create a binding
	WGPUBindGroupEntry binding = {0};
	binding.binding = 0;
	binding.buffer = uniformBuffer;
	binding.offset = 0;
	binding.size = sizeof(MyUniforms);

	// A bind group contains one or multiple bindings
	WGPUBindGroupDescriptor bindGroupDesc = {0};
	bindGroupDesc.layout = bindGroupLayout;
	bindGroupDesc.entryCount = 1;
	bindGroupDesc.entries = &binding;
	bindGroupDesc.label = toWgpuStringView("My Bind Group");
	bindGroup = wgpuDeviceCreateBindGroup(device, &bindGroupDesc);

	if (bindGroup == nullptr) throw std::runtime_error("Could not create WebGPU bind group");
}

void InitWebGPU(ES::Engine::Core &core) {
	CreateInstance(core);
	CreateSurface(core);
	CreateAdapter(core);
	AdaptaterPrintLimits(core);
	AdaptaterPrintFeatures(core);
	AdaptaterPrintProperties(core);
	ReleaseInstance(core);
	RequestCapabilities(core);
	CreateDevice(core);
	CreateQueue(core);
	SetupQueueOnSubmittedWorkDone(core);
	ConfigureSurface(core);
	ReleaseAdapter(core);
	InspectDevice(core);
	InitializePipeline(core);
	InitializeBuffers(core);
	CreateBindingGroup(core);
}

WGPUTextureView GetNextSurfaceViewData(WGPUSurface &surface){
	// Get the surface texture
	WGPUSurfaceTexture surfaceTexture;
	surfaceTexture.nextInChain = nullptr;
	wgpuSurfaceGetCurrentTexture(surface, &surfaceTexture);
	if (surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal) {
		return nullptr;
	}

	// Create a view for this surface texture
	WGPUTextureViewDescriptor viewDescriptor {0};
	viewDescriptor.nextInChain = nullptr;
	viewDescriptor.label = toWgpuStringView("Surface texture view");
	viewDescriptor.format = wgpuTextureGetFormat(surfaceTexture.texture);
	viewDescriptor.dimension = WGPUTextureViewDimension_2D;
	viewDescriptor.baseMipLevel = 0;
	viewDescriptor.mipLevelCount = 1;
	viewDescriptor.baseArrayLayer = 0;
	viewDescriptor.arrayLayerCount = 1;
	viewDescriptor.aspect = WGPUTextureAspect_All;
	WGPUTextureView targetView = wgpuTextureCreateView(surfaceTexture.texture, &viewDescriptor);

	return targetView;
}

void DrawWebGPU(ES::Engine::Core &core)
{
	WGPUDevice &device = core.GetResource<WGPUDevice>();
	WGPUSurface &surface = core.GetResource<WGPUSurface>();
	WGPURenderPipeline &pipeline = core.GetResource<WGPURenderPipeline>();
	WGPUQueue &queue = core.GetResource<WGPUQueue>();

	if (device == nullptr) throw std::runtime_error("WebGPU device is not created, cannot draw.");
	if (surface == nullptr) throw std::runtime_error("WebGPU surface is not created, cannot draw.");
	if (pipeline == nullptr) throw std::runtime_error("WebGPU render pipeline is not created, cannot draw.");
	if (queue == nullptr) throw std::runtime_error("WebGPU queue is not created, cannot draw.");

	MyUniforms uniforms;

	uniforms.time = glfwGetTime();
	uniforms.color = { 0.0f, 1.0f, 0.4f, 1.0f };
	float near = 0.001f;
	float far = 100.0f;
	float ratio = 800.0f / 800.0f;
	float fov = glm::radians(45.0f);
	uniforms.projectionMatrix = glm::perspective(fov, ratio, near, far);

	glm::vec3 focalPoint(0.0, 0.0, -3.0);
	float angle2 = 3.0 * glm::pi<float>() / 4.0;
	auto R2 = glm::rotate(glm::mat4x4(1.0), -angle2, glm::vec3(1.0, 0.0, 0.0));
	auto T2 = glm::translate(glm::mat4x4(1.0), -focalPoint);
	uniforms.viewMatrix = T2 * R2;

	float angle1 = uniforms.time;
	glm::mat4x4 M(1.0);
	M = glm::rotate(M, angle1, glm::vec3(0.0, 0.0, 1.0));
	M = glm::translate(M, glm::vec3(0.5, 0.0, 0.0));
	M = glm::scale(M, glm::vec3(0.2f));
	uniforms.modelMatrix = M;
	wgpuQueueWriteBuffer(queue, uniformBuffer, offsetof(MyUniforms, time), &uniforms.time, sizeof(MyUniforms::time));
	wgpuQueueWriteBuffer(queue, uniformBuffer, offsetof(MyUniforms, color), &uniforms.color, sizeof(MyUniforms::color));
	wgpuQueueWriteBuffer(queue, uniformBuffer, offsetof(MyUniforms, modelMatrix), &uniforms.modelMatrix, sizeof(MyUniforms::modelMatrix));
	wgpuQueueWriteBuffer(queue, uniformBuffer, offsetof(MyUniforms, viewMatrix), &uniforms.viewMatrix, sizeof(MyUniforms::viewMatrix));
	wgpuQueueWriteBuffer(queue, uniformBuffer, offsetof(MyUniforms, projectionMatrix), &uniforms.projectionMatrix, sizeof(MyUniforms::projectionMatrix));

	auto targetView = GetNextSurfaceViewData(surface);
	if (!targetView) return;

	// Create a command encoder for the draw call
	WGPUCommandEncoderDescriptor encoderDesc = {};
	encoderDesc.nextInChain = nullptr;
	encoderDesc.label = toWgpuStringView("My command encoder");
	WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDesc);

	wgpuCommandEncoderInsertDebugMarker(encoder, toWgpuStringView("Do something"));
	wgpuCommandEncoderInsertDebugMarker(encoder, toWgpuStringView("Do something else"));

	// Create the render pass that clears the screen with our color
	WGPURenderPassDescriptor renderPassDesc = {};
	renderPassDesc.nextInChain = nullptr;

	// The attachment part of the render pass descriptor describes the target texture of the pass
	WGPURenderPassColorAttachment renderPassColorAttachment = {};
	renderPassColorAttachment.view = targetView;
	renderPassColorAttachment.resolveTarget = nullptr;
	renderPassColorAttachment.loadOp = WGPULoadOp_Clear;
	renderPassColorAttachment.storeOp = WGPUStoreOp_Store;
	renderPassColorAttachment.clearValue = WGPUColor{ .05, 0.05, 0.05, 1.0 };
	renderPassColorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;

	renderPassDesc.colorAttachmentCount = 1;
	renderPassDesc.colorAttachments = &renderPassColorAttachment;
	renderPassDesc.depthStencilAttachment = nullptr;
	renderPassDesc.timestampWrites = nullptr;
	renderPassDesc.label = toWgpuStringView("My render pass");
	WGPURenderPassDepthStencilAttachment depthStencilAttachment = {0};
	depthStencilAttachment.view = depthTextureView;
	depthStencilAttachment.depthClearValue = 1.0f;
	depthStencilAttachment.depthLoadOp = WGPULoadOp_Clear;
	depthStencilAttachment.depthStoreOp = WGPUStoreOp_Store;
	renderPassDesc.depthStencilAttachment = &depthStencilAttachment;

	// Create the render pass and end it immediately (we only clear the screen but do not draw anything)
	WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);

		// Select which render pipeline to use
	wgpuRenderPassEncoderSetPipeline(renderPass, pipeline);
	wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, pointBuffer, 0, wgpuBufferGetSize(pointBuffer));
	wgpuRenderPassEncoderSetIndexBuffer(renderPass, indexBuffer, WGPUIndexFormat_Uint16, 0, wgpuBufferGetSize(indexBuffer));

	// Set binding group
	wgpuRenderPassEncoderSetBindGroup(renderPass, 0, bindGroup, 0, nullptr);
	wgpuRenderPassEncoderDrawIndexed(renderPass, indexCount, 1, 0, 0, 0);

	wgpuRenderPassEncoderEnd(renderPass);
	wgpuRenderPassEncoderRelease(renderPass);

	// Finally encode and submit the render pass
	WGPUCommandBufferDescriptor cmdBufferDescriptor = {};
	cmdBufferDescriptor.nextInChain = nullptr;
	cmdBufferDescriptor.label = toWgpuStringView("Command buffer");
	WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmdBufferDescriptor);
	wgpuCommandEncoderRelease(encoder);


	// std::cout << "Submitting command..." << std::endl;
	wgpuQueueSubmit(queue, 1, &command);
	wgpuCommandBufferRelease(command);
	// std::cout << "Command submitted." << std::endl;

	// At the end of the frame
	wgpuTextureViewRelease(targetView);
	wgpuSurfacePresent(surface);
}

void ReleaseDevice(ES::Engine::Core &core)
{
	WGPUDevice &device = core.GetResource<WGPUDevice>();

	if (device) {
		wgpuDeviceRelease(device);
		device = nullptr;
	}
}

void ReleaseSurface(ES::Engine::Core &core)
{
	WGPUSurface &surface = core.GetResource<WGPUSurface>();

	if (surface) {
		wgpuSurfaceUnconfigure(surface);
		wgpuSurfaceRelease(surface);
		surface = nullptr;
	}
}

void ReleaseQueue(ES::Engine::Core &core)
{
	WGPUQueue &queue = core.GetResource<WGPUQueue>();

	if (queue) {
		wgpuQueueRelease(queue);
		queue = nullptr;
	}
}

void ReleasePipeline(ES::Engine::Core &core)
{
	WGPURenderPipeline &pipeline = core.GetResource<WGPURenderPipeline>();

	if (pipeline) {
		wgpuRenderPipelineRelease(pipeline);
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

		RegisterSystems<ES::Engine::Scheduler::Startup>(
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
		RegisterSystems<ES::Engine::Scheduler::Update>(
			DrawWebGPU
		);
		RegisterSystems<ES::Engine::Scheduler::Shutdown>(
			ReleaseDevice,
			ReleaseSurface,
			ReleaseQueue,
			ReleasePipeline
		);
	}
};
}

auto main(int ac, char **av) -> int
{
	ES::Engine::Core core;

	core.AddPlugins<ES::Plugin::WebGPU::Plugin>();

	core.RunCore();

	return 0;
}
