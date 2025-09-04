#include "WebGPU.hpp"
#include "UpdateBuffers.hpp"
#include "structs.hpp"
#include "resource/window/Window.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

namespace ES::Plugin::WebGPU::System {

void UpdateBuffers(ES::Engine::Core &core)
{
	wgpu::Device &device = core.GetResource<wgpu::Device>();
	wgpu::Surface &surface = core.GetResource<wgpu::Surface>();
	wgpu::Queue &queue = core.GetResource<wgpu::Queue>();
	auto &window = core.GetResource<ES::Plugin::Window::Resource::Window>();

	MyUniforms uniforms;

	uniforms.time = glfwGetTime();
	uniforms.color = { 1.f, 1.0f, 1.0f, 1.0f };

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

	uniforms.cameraPosition = cameraData.position;

	queue.writeBuffer(uniformBuffer, offsetof(MyUniforms, time), &uniforms.time, sizeof(MyUniforms::time));
	queue.writeBuffer(uniformBuffer, offsetof(MyUniforms, color), &uniforms.color, sizeof(MyUniforms::color));
	queue.writeBuffer(uniformBuffer, offsetof(MyUniforms, viewMatrix), &uniforms.viewMatrix, sizeof(MyUniforms::viewMatrix));
	queue.writeBuffer(uniformBuffer, offsetof(MyUniforms, projectionMatrix), &uniforms.projectionMatrix, sizeof(MyUniforms::projectionMatrix));
	queue.writeBuffer(uniformBuffer, offsetof(MyUniforms, cameraPosition), &uniforms.cameraPosition, sizeof(MyUniforms::cameraPosition));

	auto &lights = core.GetResource<std::vector<Light>>();
	uint32_t lightsCount = static_cast<uint32_t>(lights.size());

	for (auto &light : lights) {
		if (light.type != Light::Type::Directional) continue;
		auto &additionalDirectionalLight = additionalDirectionalLights[light.lightIndex];
		
		glm::vec3 lightDirection = glm::normalize(light.direction);
		glm::vec3 posOfLight = -lightDirection * 50.0f;
		
		glm::mat4 lightProjection = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, 0.1f, 60.0f);
		
		glm::vec3 target = glm::vec3(0.0f, 1.0f, 0.0f);
		glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
		
		glm::mat4 lightView = glm::lookAt(posOfLight, target, up);
		glm::mat4 lightSpaceMatrix = lightProjection * lightView;
		
		additionalDirectionalLight.lightViewProj = lightSpaceMatrix;
		light.lightViewProjMatrix = lightSpaceMatrix;
		queue.writeBuffer(additionalDirectionalLight.buffer, 0, &additionalDirectionalLight.lightViewProj, sizeof(glm::mat4));
	}
	queue.writeBuffer(lightsBuffer, 0, &lightsCount, sizeof(uint32_t));
	queue.writeBuffer(lightsBuffer, sizeof(uint32_t) + 12 /* (padding) */, lights.data(), sizeof(Light) * lights.size());

	Uniforms2D uniforms2D;

	glm::ivec2 windowSize = window.GetSize();

	uniforms2D.orthoMatrix = glm::ortho(
		windowSize.x * -0.5f,
		windowSize.x * 0.5f,
		windowSize.y * -0.5f,
		windowSize.y * 0.5f);

	queue.writeBuffer(uniform2DBuffer, 0, &uniforms2D, sizeof(Uniforms2D));
}
}
