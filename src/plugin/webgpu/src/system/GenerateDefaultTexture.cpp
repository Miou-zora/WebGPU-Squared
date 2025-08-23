
#include "ConfigureSurface.hpp"
#include "WebGPU.hpp"
#include "Window.hpp"

void ES::Plugin::WebGPU::System::GenerateDefaultTexture(ES::Engine::Core &core) {
	auto &textureManager = core.GetResource<TextureManager>();
	auto &pipelines = core.GetResource<Pipelines>();
	textureManager.Add(entt::hashed_string("DEFAULT_TEXTURE"), core.GetResource<wgpu::Device>(), glm::uvec2(2, 2), [](glm::uvec2 pos) {
		glm::u8vec4 color;
		color.r = ((pos.x + pos.y) % 2 == 0) ? 255 : 0;
		color.g = 0;
		color.b = ((pos.x + pos.y) % 2 == 0) ? 255 : 0;
		color.a = 255;
		return color;
	}, pipelines.renderPipelines["2D"].bindGroupLayouts[1]);
}
