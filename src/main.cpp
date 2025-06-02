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
    // A simple structure holding the local information shared with the
    // onAdapterRequestEnded callback.
    struct UserData {
        WGPUAdapter adapter = nullptr;
        bool requestEnded = false;
    };
    UserData userData;

    // Callback called by wgpuInstanceRequestAdapter when the request returns
    // This is a C++ lambda function, but could be any function defined in the
    // global scope. It must be non-capturing (the brackets [] are empty) so
    // that it behaves like a regular C function pointer, which is what
    // wgpuInstanceRequestAdapter expects (WebGPU being a C API). The workaround
    // is to convey what we want to capture through the pUserData pointer,
    // provided as the last argument of wgpuInstanceRequestAdapter and received
    // by the callback as its last argument.
    auto onAdapterRequestEnded = [](
		WGPURequestAdapterStatus status, // a success status
		WGPUAdapter adapter, // the returned adapter
		WGPUStringView message, // error message, or nullptr
		WGPU_NULLABLE void* userdata1, // custom user data, as provided when requesting the adapter
		WGPU_NULLABLE void* userdata2 // custom user data, as provided when requesting the adapter
	) {
        UserData& userData = *reinterpret_cast<UserData*>(userdata1);
        if (status == WGPURequestAdapterStatus_Success) {
			userData.adapter = adapter;
		} else {
			std::cout << "Could not get WebGPU adapter: " << toStdStringView(message) << std::endl;
        }
        userData.requestEnded = true;
    };

    WGPURequestAdapterCallbackInfo callbackInfo = {};
    callbackInfo.callback = onAdapterRequestEnded;
    callbackInfo.userdata1 = &userData;

	wgpuInstanceRequestAdapter(
		instance,
		nullptr,
		callbackInfo
	);

    while (!userData.requestEnded) {
		// Hand the execution to the WebGPU instance so that it can check for
		// pending async operations, in which case it invokes our callbacks.
		wgpuInstanceProcessEvents(instance);

		// Waiting for 200ms to avoid asking too often to process events
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
		std::cout << "Adapter limits:" << std::endl;
		std::cout << " - maxTextureDimension1D: " << supportedLimits.maxTextureDimension1D << std::endl;
		std::cout << " - maxTextureDimension2D: " << supportedLimits.maxTextureDimension2D << std::endl;
		std::cout << " - maxTextureDimension3D: " << supportedLimits.maxTextureDimension3D << std::endl;
		std::cout << " - maxTextureArrayLayers: " << supportedLimits.maxTextureArrayLayers << std::endl;
	}
}

void PrintFeatures(const WGPUAdapter &adapter)
{
	// Call the function a first time with a null return address, just to get
	// the entry count.
	WGPUSupportedFeatures features = {};
	wgpuAdapterGetFeatures(adapter, &features);

	std::cout << "Adapter features:" << std::endl;
	std::cout << std::hex; // Write integers as hexadecimal to ease comparison with webgpu.h literals
	for (size_t i = 0; i < features.featureCount; ++i) {
		std::cout << " - 0x" << features.features[i] << std::endl;
	}
	std::cout << std::dec; // Restore decimal numbers

	// Free the memory that had potentially been allocated by wgpuAdapterGetFeatures()
	wgpuSupportedFeaturesFreeMembers(features);
}

void PrintProperties(const WGPUAdapter &adapter)
{
	WGPUAdapterInfo properties = {};
	properties.nextInChain = nullptr;
	wgpuAdapterGetInfo(adapter, &properties);

	std::cout << "Adapter properties:" << std::endl;
	std::cout << " - vendorID: " << properties.vendorID << std::endl;
	std::cout << " - vendorName: " << toStdStringView(properties.vendor) << std::endl;
	std::cout << " - architecture: " << toStdStringView(properties.architecture) << std::endl;
	std::cout << " - deviceID: " << properties.deviceID << std::endl;
	std::cout << " - name: " << toStdStringView(properties.device) << std::endl;
	std::cout << " - driverDescription: " << toStdStringView(properties.description) << std::endl;
	std::cout << std::hex;
	std::cout << " - adapterType: 0x" << properties.adapterType << std::endl;
	std::cout << " - backendType: 0x" << properties.backendType << std::endl;
	std::cout << std::dec; // Restore decimal numbers
	wgpuAdapterInfoFreeMembers(properties);
}

/**
 * Utility function to get a WebGPU device, so that
 *     WGPUDevice device = requestDeviceSync(adapter, options);
 * is roughly equivalent to
 *     const device = await adapter.requestDevice(descriptor);
 * It is very similar to requestAdapter
 */
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
            std::cout << "Could not get WebGPU device: " << std::string_view(message.data, message.length) << std::endl;
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
		std::cerr << "Request device timed out" << std::endl;
		throw std::runtime_error("Request device timed out");
	}

    return userData.device;
}

// We also add an inspect device function:
void InspectDevice(WGPUDevice device) {
    WGPUSupportedFeatures features = {};
    wgpuDeviceGetFeatures(device, &features);
    std::cout << "Device features:" << std::endl;
    std::cout << std::hex;
    for (size_t i = 0; i < features.featureCount; ++i) {
        std::cout << " - 0x" << features.features[i] << std::endl;
    }
    std::cout << std::dec;
    wgpuSupportedFeaturesFreeMembers(features);

    WGPULimits limits = {};
    limits.nextInChain = nullptr;

    bool success = wgpuDeviceGetLimits(device, &limits) == WGPUStatus_Success;

    if (success) {
        std::cout << "Device limits:" << std::endl;
        std::cout << " - maxTextureDimension1D: " << limits.maxTextureDimension1D << std::endl;
        std::cout << " - maxTextureDimension2D: " << limits.maxTextureDimension2D << std::endl;
        std::cout << " - maxTextureDimension3D: " << limits.maxTextureDimension3D << std::endl;
        std::cout << " - maxTextureArrayLayers: " << limits.maxTextureArrayLayers << std::endl;
    }
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

void InitializePipeline(WGPUDevice device, WGPUTextureFormat &surfaceFormat, WGPURenderPipeline &pipeline) {
	// Load the shader module
	WGPUShaderModuleDescriptor shaderDesc{};

    WGPUShaderSourceWGSL wgsl_desc = {};
    wgsl_desc.chain.sType = WGPUSType_ShaderSourceWGSL;
    wgsl_desc.code = toWgpuStringView(shaderSource);

	shaderDesc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&wgsl_desc);
	shaderDesc.label = toWgpuStringView("My shader module");
	WGPUShaderModule shaderModule = wgpuDeviceCreateShaderModule(device, &shaderDesc);

	// Create the render pipeline
	WGPURenderPipelineDescriptor pipelineDesc{};
	pipelineDesc.nextInChain = nullptr;

	// We do not use any vertex buffer for this first simplistic example
	pipelineDesc.vertex.bufferCount = 0;
	pipelineDesc.vertex.buffers = nullptr;

	// NB: We define the 'shaderModule' in the second part of this chapter.
	// Here we tell that the programmable vertex shader stage is described
	// by the function called 'vs_main' in that module.
	pipelineDesc.vertex.module = shaderModule;
	pipelineDesc.vertex.entryPoint = toWgpuStringView("vs_main");
	pipelineDesc.vertex.constantCount = 0;
	pipelineDesc.vertex.constants = nullptr;

	// Each sequence of 3 vertices is considered as a triangle
	pipelineDesc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
	
	// We'll see later how to specify the order in which vertices should be
	// connected. When not specified, vertices are considered sequentially.
	pipelineDesc.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
	
	// The face orientation is defined by assuming that when looking
	// from the front of the face, its corner vertices are enumerated
	// in the counter-clockwise (CCW) order.
	pipelineDesc.primitive.frontFace = WGPUFrontFace_CCW;
	
	// But the face orientation does not matter much because we do not
	// cull (i.e. "hide") the faces pointing away from us (which is often
	// used for optimization).
	pipelineDesc.primitive.cullMode = WGPUCullMode_None;

	// We tell that the programmable fragment shader stage is described
	// by the function called 'fs_main' in the shader module.
	WGPUFragmentState fragmentState{};
	fragmentState.module = shaderModule;
	fragmentState.entryPoint = toWgpuStringView("fs_main");
	fragmentState.constantCount = 0;
	fragmentState.constants = nullptr;

	WGPUBlendState blendState{};
	blendState.color.srcFactor = WGPUBlendFactor_SrcAlpha;
	blendState.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
	blendState.color.operation = WGPUBlendOperation_Add;
	blendState.alpha.srcFactor = WGPUBlendFactor_Zero;
	blendState.alpha.dstFactor = WGPUBlendFactor_One;
	blendState.alpha.operation = WGPUBlendOperation_Add;
	
	WGPUColorTargetState colorTarget{};
	colorTarget.format = surfaceFormat;
	colorTarget.blend = &blendState;
	colorTarget.writeMask = WGPUColorWriteMask_All; // We could write to only some of the color channels.
	
	// We have only one target because our render pass has only one output color
	// attachment.
	fragmentState.targetCount = 1;
	fragmentState.targets = &colorTarget;
	pipelineDesc.fragment = &fragmentState;

	// We do not use stencil/depth testing for now
	pipelineDesc.depthStencil = nullptr;

	// Samples per pixel
	pipelineDesc.multisample.count = 1;

	// Default value for the mask, meaning "all bits on"
	pipelineDesc.multisample.mask = ~0u;

	// Default value as well (irrelevant for count = 1 anyways)
	pipelineDesc.multisample.alphaToCoverageEnabled = false;

	pipelineDesc.layout = nullptr;

	pipeline = wgpuDeviceCreateRenderPipeline(device, &pipelineDesc);

	// We no longer need to access the shader module
	wgpuShaderModuleRelease(shaderModule);
}


void InitWebGPU(ES::Engine::Core &core) {
	WGPUData data;
	// We create a descriptor
	WGPUInstanceDescriptor desc = {};
	desc.nextInChain = nullptr;

	// We create the instance using this descriptor
	auto instance = wgpuCreateInstance(&desc);

	std::cout << "Creating surface..." << std::endl;
	auto window = core.GetResource<ES::Plugin::Window::Resource::Window>();
	auto glfwWindow = window.GetGLFWWindow();
	WGPUSurface surface = glfwCreateWindowWGPUSurface(instance, glfwWindow);

	if (surface == nullptr) {
		std::cerr << "Could not create surface" << std::endl;
	} else {
		std::cout << "Surface created: " << surface << std::endl;
	}

	std::cout << "Requesting adapter..." << std::endl;
	WGPURequestAdapterOptions adapterOpts = {};
	adapterOpts.nextInChain = nullptr;
	adapterOpts.compatibleSurface = surface;

	WGPUAdapter adapter = requestAdapterSync(instance, &adapterOpts);

	std::cout << "Got adapter: " << adapter << std::endl;

	// PrintLimits(adapter);
	// PrintFeatures(adapter);
	// PrintProperties(adapter);

	wgpuInstanceRelease(instance);

	WGPUSurfaceCapabilities capabilities = {};
	capabilities.nextInChain = nullptr;

	wgpuSurfaceGetCapabilities(surface, adapter, &capabilities);

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

	wgpuSurfaceConfigure(surface, &config);

	wgpuAdapterRelease(adapter);

	std::cout << "Got device: " << device << std::endl;
	InspectDevice(device);

	data.device = device;
	data.surface = surface;

	// InitializePipeline(device, surfaceFormat, data.pipeline);
	InitializePipelineV2(device, surfaceFormat, data.pipeline);

	InitializeBuffers(device, data.queue);

	core.RegisterResource<WGPUData>(std::move(data));
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
		[](ES::Engine::Core &core) {
			auto data = core.GetResource<WGPUData>();
			if (data.device) {
				wgpuDeviceRelease(data.device);
			}
			if (data.surface) {
				wgpuSurfaceUnconfigure(data.surface);
				wgpuSurfaceRelease(data.surface);
			}
			if (data.queue) {
				wgpuQueueRelease(data.queue);
			}
		}
	);

	core.RunCore();

	return 0;
}
