#include <webgpu/webgpu.h>
#include <glfw3webgpu.h>
#include <iostream>
#include <vector>
#include "Engine.hpp"
#include "PluginWindow.hpp"
#include "Window.hpp"
#include <GLFW/glfw3.h>

const char* shaderSource = R"(
struct VertexInput {
    @location(0) position: vec2f,
    @location(1) color: vec3f,
};

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) color: vec3f,
};

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    out.position = vec4f(in.position, 0.0, 1.0);
    out.color = in.color;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    return vec4f(in.color, 1.0);
}
)";

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

    WGPULimits limits = {};
    limits.nextInChain = nullptr;

    bool success = wgpuDeviceGetLimits(device, &limits) == WGPUStatus_Success;

	if (!success) throw std::runtime_error("Failed to get device limits");

	ES::Utils::Log::Info("Device limits:");
	ES::Utils::Log::Info(fmt::format(" - maxTextureDimension1D: {}", limits.maxTextureDimension1D));
	ES::Utils::Log::Info(fmt::format(" - maxTextureDimension2D: {}", limits.maxTextureDimension2D));
	ES::Utils::Log::Info(fmt::format(" - maxTextureDimension3D: {}", limits.maxTextureDimension3D));
	ES::Utils::Log::Info(fmt::format(" - maxTextureArrayLayers: {}", limits.maxTextureArrayLayers));
}

uint32_t vertexCount = 0;
// TODO: Release them
WGPUBuffer positionBuffer = nullptr;
WGPUBuffer colorBuffer = nullptr;

void InitializeBuffers(WGPUDevice device, WGPUQueue queue)
{
	std::vector<WGPUVertexBufferLayout> vertexBufferLayouts(2);

	// We now have 2 attributes
	std::vector<WGPUVertexAttribute> vertexAttribs(2);

	// [...] Describe the position attribute
	vertexAttribs[0].shaderLocation = 0;
	vertexAttribs[0].format = WGPUVertexFormat_Float32x2;
	vertexAttribs[0].offset = 0;

	// [...] Describe the color attribute
	vertexAttribs[1].shaderLocation = 1;
	vertexAttribs[1].format = WGPUVertexFormat_Float32x3;
	vertexAttribs[1].offset = 2 * sizeof(float);

	std::vector<float> positionData = {
		-0.45f,  0.5f,
		0.45f,  0.5f,
		0.0f, -0.5f,
		0.47f, 0.47f,
		0.25f,  0.0f,
		0.69f,  0.0f
	};

	// r0,  g0,  b0, r1,  g1,  b1, ...
	std::vector<float> colorData = {
		1.0, 1.0, 0.0, // (yellow)
		1.0, 0.0, 1.0, // (magenta)
		0.0, 1.0, 1.0, // (cyan)
		1.0, 0.0, 0.0, // (red)
		0.0, 1.0, 0.0, // (green)
		0.0, 0.0, 1.0  // (blue)
	};

	vertexCount = static_cast<uint32_t>(positionData.size() / 2);
	if (vertexCount != static_cast<uint32_t>(colorData.size() / 3)) throw std::runtime_error("Vertex count does not match color count");
	// Create vertex buffer
	WGPUBufferDescriptor bufferDesc = {
		NULL,
		toWgpuStringView(NULL),
		WGPUBufferUsage_None,
		0,
		false
	};
	bufferDesc.nextInChain = nullptr;
	bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
	bufferDesc.mappedAtCreation = false;

	bufferDesc.label = toWgpuStringView("Vertex Position");
	bufferDesc.size = positionData.size() * sizeof(float);
	positionBuffer = wgpuDeviceCreateBuffer(device, &bufferDesc);
	wgpuQueueWriteBuffer(queue, positionBuffer, 0, positionData.data(), bufferDesc.size);

	bufferDesc.label = toWgpuStringView("Vertex Color");
	bufferDesc.size = colorData.size() * sizeof(float);
	colorBuffer = wgpuDeviceCreateBuffer(device, &bufferDesc);
	wgpuQueueWriteBuffer(queue, colorBuffer, 0, colorData.data(), bufferDesc.size);
}

void InitializePipelineV2(ES::Engine::Core &core)
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

	std::vector<WGPUVertexBufferLayout> vertexBufferLayouts(2);

    WGPUVertexAttribute positionAttrib;
	positionAttrib.shaderLocation = 0; // @location(0)
	positionAttrib.format = WGPUVertexFormat_Float32x2; // size of position
	positionAttrib.offset = 0;

	vertexBufferLayouts[0].attributeCount = 1;
	vertexBufferLayouts[0].attributes = &positionAttrib;
	vertexBufferLayouts[0].arrayStride = 2 * sizeof(float); // stride = size of position
	vertexBufferLayouts[0].stepMode = WGPUVertexStepMode_Vertex;

	WGPUVertexAttribute colorAttrib;
	colorAttrib.shaderLocation = 1; // @location(1)
	colorAttrib.format = WGPUVertexFormat_Float32x3; // size of color
	colorAttrib.offset = 0;

	vertexBufferLayouts[1].attributeCount = 1;
	vertexBufferLayouts[1].attributes = &colorAttrib;
	vertexBufferLayouts[1].arrayStride = 3 * sizeof(float); // stride = size of color
	vertexBufferLayouts[1].stepMode = WGPUVertexStepMode_Vertex;

    // When describing the render pipeline:
    pipelineDesc.vertex.bufferCount = static_cast<uint32_t>(vertexBufferLayouts.size());
    pipelineDesc.vertex.buffers = vertexBufferLayouts.data();
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
	config.width = 640;
	config.height = 480;
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
	InitializePipelineV2(core);

	WGPUDevice &device = core.GetResource<WGPUDevice>();
	WGPUQueue &queue = core.GetResource<WGPUQueue>();
	InitializeBuffers(device, queue);
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
	WGPUTextureViewDescriptor viewDescriptor;
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

	// Create the render pass and end it immediately (we only clear the screen but do not draw anything)
	WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);

		// Select which render pipeline to use
	wgpuRenderPassEncoderSetPipeline(renderPass, pipeline);

	wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, positionBuffer, 0, wgpuBufferGetSize(positionBuffer));
	wgpuRenderPassEncoderSetVertexBuffer(renderPass, 1, colorBuffer, 0, wgpuBufferGetSize(colorBuffer));

	// Draw 1 instance of a 3-vertices shape
	wgpuRenderPassEncoderDraw(renderPass, vertexCount, 1, 0, 0);

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

auto main(int ac, char **av) -> int
{
	ES::Engine::Core core;

	core.AddPlugins<ES::Plugin::Window::Plugin>();

	core.RegisterSystem<ES::Engine::Scheduler::Startup>(
		InitWebGPU
	);

	core.RegisterSystem<ES::Engine::Scheduler::Update>(
		DrawWebGPU
	);

	core.RegisterSystem<ES::Engine::Scheduler::Shutdown>(
		ReleaseDevice,
		ReleaseSurface,
		ReleaseQueue,
		ReleasePipeline
	);

	core.RunCore();

	return 0;
}
