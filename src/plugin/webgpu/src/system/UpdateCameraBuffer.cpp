
#include "ConfigureSurface.hpp"
#include "WebGPU.hpp"
#include "Window.hpp"

void ES::Plugin::WebGPU::System::UpdateCameraBuffer(ES::Engine::Core &core) {
	wgpu::Queue queue = core.GetResource<wgpu::Queue>();
	auto &camData = core.GetResource<CameraData>();

	auto viewMatrix = glm::lookAt(
		camData.position,
		camData.position + glm::vec3(
			glm::cos(camData.yaw) * glm::cos(camData.pitch),
			glm::sin(camData.pitch),
			glm::sin(camData.yaw) * glm::cos(camData.pitch)
		),
		camData.up
	);

	auto projectionMatrix = glm::perspective(
		camData.fovY,
		camData.aspectRatio,
		camData.nearPlane,
		camData.farPlane
	);
	Camera cameraBuf;
	cameraBuf.viewProjectionMatrix = projectionMatrix * viewMatrix;
	cameraBuf.invViewProjectionMatrix = glm::inverse(cameraBuf.viewProjectionMatrix);
	cameraBuf.position = camData.position;
	queue.writeBuffer(cameraBuffer, 0, &cameraBuf, sizeof(cameraBuf));
}
