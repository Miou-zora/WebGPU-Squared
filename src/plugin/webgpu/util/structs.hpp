#pragma once

#include "Texture.hpp"
#include <glm/glm.hpp>
#include "webgpu.hpp"
#include "Object.hpp"

#include "stb_image.h"

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
	std::vector<wgpu::BindGroupLayout> bindGroupLayouts;
	wgpu::PipelineLayout layout = nullptr;
};

using TextureManager = ES::Plugin::Object::Resource::ResourceManager<Texture>;
struct Pipelines {
	std::map<std::string, PipelineData> renderPipelines;
};

struct Uniforms2D {
	glm::mat4 orthoMatrix;
};

constexpr size_t MAX_LIGHTS = 16;

inline wgpu::Buffer uniformBuffer = nullptr;
inline wgpu::Buffer uniform2DBuffer = nullptr;
inline wgpu::Buffer lightsBuffer = nullptr;
inline wgpu::TextureFormat depthTextureFormat = wgpu::TextureFormat::Depth24Plus;
inline wgpu::Texture textureToRelease = nullptr;
inline wgpu::TextureView textureView = nullptr;
inline wgpu::TextureView depthTextureView = nullptr;

struct BindGroupsLinks {
	enum class AssetType {
		BindGroup, // often used for pur data like uniforms
		TextureView,
	} type;
	std::string name; // In case of usage global
	uint32_t groupIndex; // Index of the bind group in the pipeline
};

struct RenderPassData {
	std::string name;
	std::optional<std::string> pipelineName;
	wgpu::LoadOp loadOp = wgpu::LoadOp::Load;
	std::optional<std::function<glm::vec4(ES::Engine::Core &)>> clearColor; // 0 to 1 range, nullptr if load operation is not clear
	std::string outputColorTextureName;
	std::string outputDepthTextureName;
	std::vector<BindGroupsLinks> bindGroups;
	std::optional<std::function<void(wgpu::RenderPassEncoder &renderPass, ES::Engine::Core &core)>> uniqueRenderCallback = std::nullopt;
	std::optional<std::function<void(wgpu::RenderPassEncoder &renderPass, ES::Engine::Core &core, ES::Plugin::WebGPU::Component::Mesh &, ES::Plugin::Object::Component::Transform &, ES::Engine::Entity)>> perEntityCallback;
};
