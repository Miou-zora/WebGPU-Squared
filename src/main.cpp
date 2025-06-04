#include <webgpu/webgpu.h>
#include <glfw3webgpu.h>
#include <iostream>
#include <vector>
#include "Engine.hpp"
#include "PluginWindow.hpp"
#include "Window.hpp"
#include <GLFW/glfw3.h>

struct WGPUData {
	WGPUDevice device = nullptr;
	WGPUSurface surface = nullptr;
	WGPUQueue queue = nullptr;
	WGPURenderPipeline pipeline = nullptr;
};

const char* shaderSource = R"(
@vertex
fn vs_main(@location(0) in_vertex_position: vec2f) -> @builtin(position) vec4f {
	return vec4f(in_vertex_position, 0.0, 1.0);
}

@fragment
fn fs_main() -> @location(0) vec4f {
	return vec4f(0.0, 0.4, 1.0, 1.0);
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

void PrintLimits(const WGPUAdapter &adapter)
{
	WGPULimits supportedLimits = {};
	supportedLimits.nextInChain = nullptr;

	WGPUStatus success = wgpuAdapterGetLimits(adapter, &supportedLimits);

	if (success == WGPUStatus_Success) {
		ES::Utils::Log::Info("Adapter limits:");
		ES::Utils::Log::Info(fmt::format(" - maxTextureDimension1D: {}", supportedLimits.maxTextureDimension1D));
		ES::Utils::Log::Info(fmt::format(" - maxTextureDimension2D: {}", supportedLimits.maxTextureDimension2D));
		ES::Utils::Log::Info(fmt::format(" - maxTextureDimension3D: {}", supportedLimits.maxTextureDimension3D));
		ES::Utils::Log::Info(fmt::format(" - maxTextureArrayLayers: {}", supportedLimits.maxTextureArrayLayers));
	}
}

void PrintFeatures(const WGPUAdapter &adapter)
{
	WGPUSupportedFeatures features = {};
	wgpuAdapterGetFeatures(adapter, &features);

	ES::Utils::Log::Info("Adapter features:");
	for (size_t i = 0; i < features.featureCount; ++i) {
		ES::Utils::Log::Info(fmt::format(" - 0x{:x}", static_cast<uint32_t>(features.features[i])));
	}
	wgpuSupportedFeaturesFreeMembers(features);
}

void PrintProperties(const WGPUAdapter &adapter)
{
	WGPUAdapterInfo properties = {};
	properties.nextInChain = nullptr;
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

void InspectDevice(WGPUDevice device) {
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

	if (!success) {
		ES::Utils::Log::Error("Failed to get device limits");
		throw std::runtime_error("Failed to get device limits");
	}

	ES::Utils::Log::Info("Device limits:");
	ES::Utils::Log::Info(fmt::format(" - maxTextureDimension1D: {}", limits.maxTextureDimension1D));
	ES::Utils::Log::Info(fmt::format(" - maxTextureDimension2D: {}", limits.maxTextureDimension2D));
	ES::Utils::Log::Info(fmt::format(" - maxTextureDimension3D: {}", limits.maxTextureDimension3D));
	ES::Utils::Log::Info(fmt::format(" - maxTextureArrayLayers: {}", limits.maxTextureArrayLayers));
}

uint32_t vertexCount = 0;
WGPUBuffer vertexBuffer = nullptr;

void InitializeBuffers(WGPUDevice device, WGPUQueue queue)
{
	std::vector<float> vertexData = {
		// Define a first triangle:
		-0.45f, 0.5f,
		0.45f, 0.5f,
		0.0f, -0.5f,
	
		// Add a second triangle:
		0.47f, 0.47f,
		0.25f, 0.0f,
		0.69f, 0.0f
	};
	
	vertexCount = static_cast<uint32_t>(vertexData.size() / 2);
	// Create vertex buffer
	WGPUBufferDescriptor bufferDesc = {
		NULL,
		toWgpuStringView(NULL),
		WGPUBufferUsage_None,
		0,
		false
	};
	bufferDesc.size = vertexData.size() * sizeof(float);
	bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex; // Vertex usage here!
	vertexBuffer = wgpuDeviceCreateBuffer(device, &bufferDesc);
	
	// Upload geometry data to the buffer
	wgpuQueueWriteBuffer(queue, vertexBuffer, 0, vertexData.data(), bufferDesc.size);
}

void InitializePipelineV2(WGPUDevice device, WGPUTextureFormat surfaceFormat, WGPURenderPipeline &pipeline)
{
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
    WGPUVertexAttribute positionAttrib = {
		(WGPUVertexFormat)0,
		0,
		0
	};
    // == For each attribute, describe its layout, i.e., how to interpret the raw data ==
    // Corresponds to @location(...)
    positionAttrib.shaderLocation = 0;
    // Means vec2f in the shader
    positionAttrib.format = WGPUVertexFormat_Float32x2;
    // Index of the first element
    positionAttrib.offset = 0;
    vertexBufferLayout.attributeCount = 1;
    vertexBufferLayout.attributes = &positionAttrib;
    // == Common to attributes from the same buffer ==
    vertexBufferLayout.arrayStride = 2 * sizeof(float);
    vertexBufferLayout.stepMode = WGPUVertexStepMode_Vertex;
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
    pipeline = wgpuDeviceCreateRenderPipeline(device, &pipelineDesc);
    wgpuShaderModuleRelease(shaderModule);
}

void CreateSurface(ES::Engine::Core &core) {
	WGPUData &data = core.GetResource<WGPUData>();
	auto &instance = core.GetResource<WGPUInstance>();
	ES::Utils::Log::Info("Creating surface...");
	auto window = core.GetResource<ES::Plugin::Window::Resource::Window>();
	auto glfwWindow = window.GetGLFWWindow();
	data.surface = glfwCreateWindowWGPUSurface(instance, glfwWindow);

	if (data.surface == nullptr) {
		ES::Utils::Log::Error("Could not create surface");
	} else {
		ES::Utils::Log::Info(fmt::format("Surface created: {}", static_cast<void*>(data.surface)));
	}
}

void CreateInstance(ES::Engine::Core &core) {
	WGPUInstanceDescriptor desc = {};
	desc.nextInChain = nullptr;
	core.RegisterResource<WGPUInstance>(wgpuCreateInstance(&desc));
}

void InitWebGPU(ES::Engine::Core &core) {
	WGPUData &data = core.RegisterResource(WGPUData{});

	CreateInstance(core);
	WGPUInstance &instance = core.GetResource<WGPUInstance>();

	CreateSurface(core);

	ES::Utils::Log::Info("Requesting adapter...");
	WGPURequestAdapterOptions adapterOpts = {};
	adapterOpts.nextInChain = nullptr;
	adapterOpts.compatibleSurface = data.surface;

	WGPUAdapter adapter = requestAdapterSync(instance, &adapterOpts);

	ES::Utils::Log::Info(fmt::format("Got adapter: {}", static_cast<void*>(adapter)));

	PrintLimits(adapter);
	PrintFeatures(adapter);
	PrintProperties(adapter);

	wgpuInstanceRelease(instance);

	WGPUSurfaceCapabilities capabilities = {};
	capabilities.nextInChain = nullptr;

	wgpuSurfaceGetCapabilities(data.surface, adapter, &capabilities);

	std::cout << "Requesting device..." << std::endl;

	WGPUDeviceDescriptor deviceDesc = {};

	deviceDesc.nextInChain = nullptr;
	deviceDesc.label = toWgpuStringView("My Device"); // anything works here, that's your call
	deviceDesc.requiredFeatureCount = 0; // we do not require any specific feature
	deviceDesc.requiredLimits = nullptr; // we do not require any specific limit (it will choose the minimal ones)
	deviceDesc.defaultQueue.nextInChain = nullptr;
	deviceDesc.defaultQueue.label = toWgpuStringView("The default queue");
	deviceDesc.deviceLostCallbackInfo = {};
	deviceDesc.deviceLostCallbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;
	deviceDesc.deviceLostCallbackInfo.callback = [](WGPUDevice const * device, WGPUDeviceLostReason reason, WGPUStringView message, WGPU_NULLABLE void* userdata1, WGPU_NULLABLE void* userdata2) {
		std::cout << "Device lost: reason " << reason << " (" << toStdStringView(message) << ")" << std::endl;
	};
	deviceDesc.uncapturedErrorCallbackInfo = {};
	deviceDesc.uncapturedErrorCallbackInfo.callback = [](WGPUDevice const * device, WGPUErrorType type, WGPUStringView message, WGPU_NULLABLE void* userdata1, WGPU_NULLABLE void* userdata2) {
		std::cout << "Uncaptured device error: type " << type << " (" << toStdStringView(message) << ")" << std::endl;
	};

	WGPUDevice device = requestDeviceSync(adapter, &deviceDesc);

	data.queue = wgpuDeviceGetQueue(device);
	auto onQueueWorkDone = [](WGPUQueueWorkDoneStatus status, WGPU_NULLABLE void* userdata1, WGPU_NULLABLE void* userdata2) {
		std::cout << "Queued work finished with status: " << status << std::endl;
	};
	WGPUQueueWorkDoneCallbackInfo callbackInfo = {};
	callbackInfo.callback = onQueueWorkDone;
	callbackInfo.userdata1 = nullptr;
	callbackInfo.userdata2 = nullptr;
	wgpuQueueOnSubmittedWorkDone(data.queue, callbackInfo);


	WGPUSurfaceConfiguration config = {};
	config.nextInChain = nullptr;
	config.width = 640;
	config.height = 480;
	config.usage = WGPUTextureUsage_RenderAttachment;

	WGPUTextureFormat surfaceFormat = capabilities.formats[0];
	config.format = surfaceFormat;
	config.viewFormatCount = 0;
	config.viewFormats = nullptr;

	config.device = device;
	config.presentMode = WGPUPresentMode_Fifo;
	config.alphaMode = WGPUCompositeAlphaMode_Auto;

	wgpuSurfaceConfigure(data.surface, &config);

	wgpuAdapterRelease(adapter);

	std::cout << "Got device: " << device << std::endl;
	InspectDevice(device);

	data.device = device;

	// InitializePipeline(device, surfaceFormat, data.pipeline);
	InitializePipelineV2(device, surfaceFormat, data.pipeline);

	InitializeBuffers(device, data.queue);

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
	auto &data = core.GetResource<WGPUData>();
	auto targetView = GetNextSurfaceViewData(data.surface);
	if (!targetView) return;

	// Create a command encoder for the draw call
	WGPUCommandEncoderDescriptor encoderDesc = {};
	encoderDesc.nextInChain = nullptr;
	encoderDesc.label = toWgpuStringView("My command encoder");
	WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(data.device, &encoderDesc);

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
	renderPassColorAttachment.clearValue = WGPUColor{ 0.9, 0.1, 0.2, 1.0 };
	renderPassColorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;

	renderPassDesc.colorAttachmentCount = 1;
	renderPassDesc.colorAttachments = &renderPassColorAttachment;
	renderPassDesc.depthStencilAttachment = nullptr;
	renderPassDesc.timestampWrites = nullptr;
	renderPassDesc.label = toWgpuStringView("My render pass");

	// Create the render pass and end it immediately (we only clear the screen but do not draw anything)
	WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);

		// Select which render pipeline to use
	wgpuRenderPassEncoderSetPipeline(renderPass, data.pipeline);

	wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, vertexBuffer, 0, wgpuBufferGetSize(vertexBuffer));

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
	wgpuQueueSubmit(data.queue, 1, &command);
	wgpuCommandBufferRelease(command);
	// std::cout << "Command submitted." << std::endl;

	// At the end of the frame
	wgpuTextureViewRelease(targetView);
	wgpuSurfacePresent(data.surface);
}

void ReleaseDevice(ES::Engine::Core &core)
{
	auto data = core.GetResource<WGPUData>();

	if (data.device) {
		wgpuDeviceRelease(data.device);
		data.device = nullptr;
	}
}

void ReleaseSurface(ES::Engine::Core &core)
{
	auto data = core.GetResource<WGPUData>();

	if (data.surface) {
		wgpuSurfaceUnconfigure(data.surface);
		wgpuSurfaceRelease(data.surface);
		data.surface = nullptr;
	}
}

void ReleaseQueue(ES::Engine::Core &core)
{
	auto data = core.GetResource<WGPUData>();

	if (data.queue) {
		wgpuQueueRelease(data.queue);
		data.queue = nullptr;
	}
}

void ReleasePipeline(ES::Engine::Core &core)
{
	auto data = core.GetResource<WGPUData>();

	if (data.pipeline) {
		wgpuRenderPipelineRelease(data.pipeline);
		data.pipeline = nullptr;
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
