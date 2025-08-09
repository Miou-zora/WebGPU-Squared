#pragma once

#include "webgpu.hpp"
#include "stb_image.h"
#include <filesystem>
#include <glm/glm.hpp>

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
