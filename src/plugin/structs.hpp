#pragma once

#include <glm/glm.hpp>
#include "webgpu.hpp"
#include "Object.hpp"
#include <filesystem>

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

struct Texture {
	wgpu::Texture texture = nullptr;
	wgpu::TextureView textureView = nullptr;
	wgpu::Sampler sampler = nullptr;
	wgpu::BindGroup bindGroup = nullptr;
	wgpu::TextureFormat format = wgpu::TextureFormat::RGBA8Unorm;

	Texture() = default;

	Texture(wgpu::Device &device, const std::filesystem::path &path, wgpu::BindGroupLayout bindGroupLayout) {
		int width, height, channels;
	    unsigned char *pixelData = stbi_load(path.string().c_str(), &width, &height, &channels, 4 /* force 4 channels */);
		if (!pixelData) throw std::runtime_error("Failed to load texture data.");

		this->format = wgpu::TextureFormat::RGBA8UnormSrgb;
		this->texture = this->CreateTexture(device, { (uint32_t)width, (uint32_t)height });
		this->textureView = this->CreateTextureView(this->texture);

		this->WriteTexture(device, pixelData);
		stbi_image_free(pixelData);

		this->sampler = CreateSampler(device);
		this->bindGroup = CreateBindGroup(device, bindGroupLayout);
	}

	Texture(wgpu::Device &device, glm::uvec2 size, std::function<glm::u8vec4 (glm::uvec2 pos)> callback, wgpu::BindGroupLayout bindGroupLayout) {
		std::vector<uint8_t> pixels;

		this->format = wgpu::TextureFormat::RGBA8Unorm;
		this->texture = this->CreateTexture(device, { size.x, size.y });
		this->textureView = this->CreateTextureView(this->texture);

		this->GenerateTextureFromCallback(callback, pixels);
		this->WriteTexture(device, pixels.data());

		this->sampler = CreateSampler(device);
		this->bindGroup = CreateBindGroup(device, bindGroupLayout);
	}

private:

	wgpu::BindGroup CreateBindGroup(wgpu::Device &device, wgpu::BindGroupLayout bindGroupLayout) {
		wgpu::BindGroupEntry textureViewBinding(wgpu::Default);
		textureViewBinding.binding = 0;
		textureViewBinding.textureView = this->textureView;

		wgpu::BindGroupEntry samplerBinding(wgpu::Default);
		samplerBinding.binding = 1;
		samplerBinding.sampler = this->sampler;

		std::array<wgpu::BindGroupEntry, 2> bindings = { textureViewBinding, samplerBinding };

		wgpu::BindGroupDescriptor bindGroupDesc(wgpu::Default);
		bindGroupDesc.layout = bindGroupLayout;
		bindGroupDesc.entryCount = bindings.size();
		bindGroupDesc.entries = bindings.data();
		bindGroupDesc.label = wgpu::StringView("Texture Bind Group");

		return device.createBindGroup(bindGroupDesc);
	}

	wgpu::Texture CreateTexture(wgpu::Device &device, glm::uvec2 size)
	{
		wgpu::TextureDescriptor textureDesc;
		textureDesc.label = wgpu::StringView("Custom Texture");
		textureDesc.size = { size.x, size.y, 1 };
		textureDesc.dimension = wgpu::TextureDimension::_2D;
		textureDesc.mipLevelCount = 1;
		textureDesc.sampleCount = 1;
		textureDesc.format = this->format;
		textureDesc.usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst;
		textureDesc.viewFormats = nullptr;
		textureDesc.viewFormatCount = 0;
		return device.createTexture(textureDesc);
	}

	wgpu::TextureView CreateTextureView(wgpu::Texture &texture)
	{
		wgpu::TextureViewDescriptor textureViewDesc;
		textureViewDesc.arrayLayerCount = 1;
		textureViewDesc.mipLevelCount = 1;
		textureViewDesc.dimension = wgpu::TextureViewDimension::_2D;
		textureViewDesc.format = this->format;
		return texture.createView(textureViewDesc);
	}

	void WriteTexture(
		wgpu::Device device,
		const unsigned char* pixelData)
	{
		wgpu::Extent3D textureSize = { this->texture.getWidth(), this->texture.getHeight(), 1 };
		wgpu::TexelCopyTextureInfo destination;
		destination.texture = texture;
		destination.mipLevel = 0;
		destination.origin = { 0, 0, 0 };
		destination.aspect = wgpu::TextureAspect::All;

		wgpu::TexelCopyBufferLayout source;
		source.offset = 0;
		source.bytesPerRow = 4 * textureSize.width;
		source.rowsPerImage = textureSize.height;

		wgpu::Queue queue = device.getQueue();
		queue.writeTexture(destination, pixelData, 4 * textureSize.width * textureSize.height, source, textureSize);
		queue.release();
	}

	void GenerateTextureFromCallback(std::function<glm::u8vec4 (glm::uvec2 pos)> callback, std::vector<uint8_t> &pixels) {
		auto width = this->texture.getWidth();
		auto height = this->texture.getHeight();
		pixels.resize(4 * width * height);
		for (uint32_t i = 0; i < width; ++i) {
			for (uint32_t j = 0; j < height; ++j) {
				uint8_t *p = &pixels[4 * (j * width + i)];
				glm::u8vec4 color = callback(glm::uvec2(i, j));
				p[0] = color.r;
				p[1] = color.g;
				p[2] = color.b;
				p[3] = color.a;
			}
		}
	}

	wgpu::Sampler CreateSampler(wgpu::Device &device) {
		wgpu::SamplerDescriptor samplerDesc(wgpu::Default);
		samplerDesc.maxAnisotropy = 1;
		return device.createSampler(samplerDesc);
	}
};

using TextureManager = ES::Plugin::Object::Resource::ResourceManager<Texture>;

struct Sprite {
	wgpu::Buffer pointBuffer = nullptr;
	wgpu::Buffer indexBuffer = nullptr;
	uint32_t indexCount = 0;
	entt::hashed_string textureHandleID;

	bool enabled = true;

	Sprite() = default;

	Sprite(ES::Engine::Core &core, glm::vec2 position, glm::vec2 size, entt::hashed_string textureHandleID_ = "DEFAULT"/* TODO: add a default texture for sprite */):
		textureHandleID(textureHandleID_)
	{
		auto &device = core.GetResource<wgpu::Device>();
		auto &queue = core.GetResource<wgpu::Queue>();

		std::vector<float> pointData = {
			// Position.x, Position.y, Position.z, U, V
			position.x, position.y, 0.0f, 0.0f, 0.0f, // Bottom left
			position.x + size.x, position.y, 0.0f, 1.0f, 0.0f, // Bottom right
			position.x + size.x, position.y + size.y, 0.0f, 1.0f, 1.0f, // Top right
			position.x, position.y + size.y, 0.0f, 0.0f, 1.0f // Top left
		};

		std::vector<uint32_t> indexData = {
			0, 1, 2, // Triangle 1
			0, 2, 3  // Triangle 2
		};

		indexCount = indexData.size();

		wgpu::BufferDescriptor bufferDesc(wgpu::Default);
		bufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex;
		bufferDesc.mappedAtCreation = false;
		bufferDesc.size = pointData.size() * sizeof(float);
		pointBuffer = device.createBuffer(bufferDesc);

		queue.writeBuffer(pointBuffer, 0, pointData.data(), bufferDesc.size);

		bufferDesc.size = indexData.size() * sizeof(uint32_t);
		bufferDesc.size = (bufferDesc.size + 3) & ~3;
		indexData.resize((indexData.size() + 1) & ~1);
		bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index;
		indexBuffer = device.createBuffer(bufferDesc);

		queue.writeBuffer(indexBuffer, 0, indexData.data(), bufferDesc.size);
	}

	void Release() {
		if (pointBuffer) {
			pointBuffer.destroy();
			pointBuffer.release();
			pointBuffer = nullptr;
		}
		if (indexBuffer) {
			indexBuffer.destroy();
			indexBuffer.release();
			indexBuffer = nullptr;
		}
		indexCount = 0;
	}
};

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
