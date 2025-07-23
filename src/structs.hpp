#pragma once

#include <glm/glm.hpp>
#include <webgpu/webgpu.h>


struct MyUniforms {
	glm::mat4x4 projectionMatrix;
    glm::mat4x4 viewMatrix;
    glm::mat4x4 modelMatrix;
    glm::vec4 color;
    glm::vec3 cameraPosition;
    float time;
};

// This assert should stay here as we want this rule to link struct to webgpu struct
static_assert(sizeof(MyUniforms) % 16 == 0);

struct Light {
    glm::vec4 color; // 16
	glm::vec3 direction; // 16 + 12 = 28
    float intensity; // 28 + 4 = 32
	uint32_t enabled = 0; // 32 + 4 = 36
	enum class Type : uint32_t {
		Directional,
		Point
	} type = Type::Point; // 36 + 4 = 40
	char _padding[8] = {0}; // 40 + 8 = 48
};

static_assert(sizeof(Light) % 16 == 0, "Light struct must be 16 bytes for WebGPU alignment");

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

struct BindGroups {
	std::map<std::string, wgpu::BindGroup> groups;
};

struct PipelineData {
	wgpu::RenderPipeline pipeline = nullptr;
	wgpu::BindGroupLayout bindGroupLayout = nullptr;
	wgpu::PipelineLayout layout = nullptr;
	wgpu::TextureView depthTextureView = nullptr;
};

struct Pipelines {
	std::map<std::string, PipelineData> renderPipelines;
};

constexpr size_t MAX_LIGHTS = 16;

wgpu::Buffer uniformBuffer = nullptr;
wgpu::Buffer lightsBuffer = nullptr;
wgpu::TextureFormat depthTextureFormat = wgpu::TextureFormat::Depth24Plus;
wgpu::Texture textureToRelease = nullptr;
wgpu::RenderPassEncoder renderPass = nullptr;
wgpu::TextureView textureView = nullptr;
