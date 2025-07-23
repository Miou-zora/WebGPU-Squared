#pragma once

#include <webgpu/webgpu.h>


auto RequestAdapterSync(wgpu::Instance instance, wgpu::RequestAdapterOptions const &options)
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

    wgpu::RequestAdapterCallbackInfo callbackInfo(wgpu::Default);
    callbackInfo.callback = onAdapterRequestEnded;
    callbackInfo.userdata1 = &userData;

	instance.requestAdapter(wgpu::Default, callbackInfo);

    while (!userData.requestEnded) {
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}

    return userData.adapter;
}
