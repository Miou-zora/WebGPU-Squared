#include <webgpu/webgpu.h>
#include <glfw3webgpu.h>
#include <iostream>
#include <vector>
#include "Engine.hpp"
#include "PluginWindow.hpp"
#include "Window.hpp"

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
			std::cout << "Could not get WebGPU adapter: ";
			std::cout.write(message.data, message.length);
			std::cout << std::endl;
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

    if (!userData.requestEnded) {
		throw std::runtime_error("Request adapter timed out");
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
	WGPUSupportedFeatures supportedFeatures = {};
	wgpuAdapterGetFeatures(adapter, &supportedFeatures);

	std::cout << "Adapter features:" << std::endl;
	std::cout << std::hex; // Write integers as hexadecimal to ease comparison with webgpu.h literals
	for (uint32_t i = 0; i < supportedFeatures.featureCount; ++i) {
		std::cout << " - 0x" << supportedFeatures.features[i] << std::endl;
	}
	std::cout << std::dec; // Restore decimal numbers
}

void PrintProperties(const WGPUAdapter &adapter)
{
	WGPUAdapterInfo properties = {};
	properties.nextInChain = nullptr;
	wgpuAdapterGetInfo(adapter, &properties);

	std::cout << "Adapter properties:" << std::endl;
	std::cout << " - vendorID: " << properties.vendorID << std::endl;
	if (properties.vendor.data) {
		std::cout << " - vendorName: " << std::string_view(properties.vendor.data, properties.vendor.length) << std::endl;
	}
	if (properties.architecture.data) {
		std::cout << " - architecture: " << std::string_view(properties.architecture.data, properties.architecture.length) << std::endl;
	}
	std::cout << " - deviceID: " << properties.deviceID << std::endl;
	if (properties.device.data) {
		std::cout << " - name: " << std::string_view(properties.device.data, properties.device.length) << std::endl;
	}
	if (properties.description.data) {
		std::cout << " - description: " << std::string_view(properties.description.data, properties.description.length) << std::endl;
	}
	std::cout << std::hex;
	std::cout << " - adapterType: 0x" << properties.adapterType << std::endl;
	std::cout << " - backendType: 0x" << properties.backendType << std::endl;
	std::cout << std::dec; // Restore decimal numbers
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
    WGPUSupportedFeatures supportedFeatures = {};
    wgpuDeviceGetFeatures(device, &supportedFeatures);

    std::cout << "Device features:" << std::endl;
    std::cout << std::hex;
    for (uint32_t i = 0; i < supportedFeatures.featureCount; ++i) {
        std::cout << " - 0x" << supportedFeatures.features[i] << std::endl;
    }
    std::cout << std::dec;

    WGPULimits limits = {};
    limits.nextInChain = nullptr;

    WGPUStatus success = wgpuDeviceGetLimits(device, &limits);

    if (success == WGPUStatus_Success) {
        std::cout << "Device limits:" << std::endl;
        std::cout << " - maxTextureDimension1D: " << limits.maxTextureDimension1D << std::endl;
        std::cout << " - maxTextureDimension2D: " << limits.maxTextureDimension2D << std::endl;
        std::cout << " - maxTextureDimension3D: " << limits.maxTextureDimension3D << std::endl;
        std::cout << " - maxTextureArrayLayers: " << limits.maxTextureArrayLayers << std::endl;
    }
}

void InitWebGPU(ES::Engine::Core &core)
{
	// We create a descriptor
	WGPUInstanceDescriptor desc = {};
	desc.nextInChain = nullptr;

	// We create the instance using this descriptor
	auto instance = wgpuCreateInstance(&desc);

	// We can check whether there is actually an instance created
	if (!instance) {
		std::cerr << "Could not initialize WebGPU!" << std::endl;
		throw std::runtime_error("Could not initialize WebGPU");
	}

	// Display the object (WGPUInstance is a simple pointer, it may be
	// copied around without worrying about its size).
	std::cout << "WGPU instance: " << instance << std::endl;

	std::cout << "Requesting adapter..." << std::endl;
	WGPURequestAdapterOptions adapterOpts = {};
	adapterOpts.nextInChain = nullptr;

	WGPUAdapter adapter = requestAdapterSync(instance, &adapterOpts);
	wgpuInstanceRelease(instance);

	std::cout << "Got adapter: " << adapter << std::endl;

	PrintLimits(adapter);
	PrintFeatures(adapter);
	PrintProperties(adapter);

	std::cout << "Requesting device..." << std::endl;

	WGPUDeviceDescriptor deviceDesc = {};

	deviceDesc.nextInChain = nullptr;
	deviceDesc.label = WGPUStringView("My Device", 9); // anything works here, that's your call
	deviceDesc.requiredFeatureCount = 0; // we do not require any specific feature
	deviceDesc.requiredLimits = nullptr; // we do not require any specific limit (it will choose the minimal ones)
	deviceDesc.defaultQueue.nextInChain = nullptr;
	deviceDesc.defaultQueue.label = WGPUStringView("The default queue", 17);
	deviceDesc.deviceLostCallbackInfo = {};
	deviceDesc.deviceLostCallbackInfo.callback = [](WGPUDevice const * device, WGPUDeviceLostReason reason, WGPUStringView message, WGPU_NULLABLE void* userdata1, WGPU_NULLABLE void* userdata2) {
		std::cout << "Device lost: reason " << reason;
		if (message.data) std::cout << " (" << std::string_view(message.data, message.length) << ")";
		std::cout << std::endl;
	};
	deviceDesc.uncapturedErrorCallbackInfo = {};
	deviceDesc.uncapturedErrorCallbackInfo.callback = [](WGPUDevice const * device, WGPUErrorType type, WGPUStringView message, WGPU_NULLABLE void* userdata1, WGPU_NULLABLE void* userdata2) {
		std::cout << "Uncaptured device error: type " << type;
		if (message.data) std::cout << " (" << std::string_view(message.data, message.length) << ")";
		std::cout << std::endl;
	};

	WGPUDevice device = requestDeviceSync(adapter, &deviceDesc);
	// TODO: release device at the end

	std::cout << "Got device: " << device << std::endl;
	wgpuAdapterRelease(adapter);

	InspectDevice(device);

	auto window = core.GetResource<ES::Plugin::Window::Resource::Window>();

	WGPUSurface surface = glfwCreateWindowWGPUSurface(instance, window.GetGLFWWindow());
	// TODO: release surface at the end

	if (!surface) {
		std::cerr << "Could not create WebGPU surface!" << std::endl;
		throw std::runtime_error("Could not create WebGPU surface");
	}
}

auto main(int ac, char **av) -> int
{
	ES::Engine::Core core;

	core.AddPlugins<ES::Plugin::Window::Plugin>();

	core.RunCore();
	// WGPUQueue queue = wgpuDeviceGetQueue(device);

	// auto onQueueWorkDone = [](WGPUQueueWorkDoneStatus status, WGPU_NULLABLE void* userdata1, WGPU_NULLABLE void* userdata2) {
	// 	std::cout << "Queued work finished with status: " << status << std::endl;
	// };
	// WGPUQueueWorkDoneCallbackInfo callbackInfo = {};
	// callbackInfo.callback = onQueueWorkDone;
	// callbackInfo.userdata1 = nullptr;
	// callbackInfo.userdata2 = nullptr;
	// wgpuQueueOnSubmittedWorkDone(queue, callbackInfo);

	// WGPUCommandEncoderDescriptor encoderDesc = {};
	// encoderDesc.nextInChain = nullptr;
	// encoderDesc.label = WGPUStringView("My Encoder", 10);
	// WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDesc);

	// wgpuCommandEncoderInsertDebugMarker(encoder, WGPUStringView("Do something", 12));
	// wgpuCommandEncoderInsertDebugMarker(encoder, WGPUStringView("Do something else", 17));

	// WGPUCommandBufferDescriptor cmdBufferDescriptor = {};
	// cmdBufferDescriptor.nextInChain = nullptr;
	// cmdBufferDescriptor.label = WGPUStringView("Command buffer", 14);
	// WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmdBufferDescriptor);
	// wgpuCommandEncoderRelease(encoder); // release encoder after it's finished

	// // Finally submit the command queue
	// std::cout << "Submitting command..." << std::endl;
	// wgpuQueueSubmit(queue, 1, &command);
	// wgpuCommandBufferRelease(command);
	// std::cout << "Command submitted." << std::endl;

	// wgpuQueueRelease(queue);

	// wgpuDeviceRelease(device);
	return 0;
}
