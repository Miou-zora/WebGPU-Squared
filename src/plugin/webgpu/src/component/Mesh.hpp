#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "util/webgpu.hpp"
#include <entt/entt.hpp>
#include "core/Core.hpp"
#include "PipelineType.hpp"

namespace ES::Plugin::WebGPU::Component {
struct Mesh {
	wgpu::Buffer pointBuffer = nullptr;
	wgpu::Buffer indexBuffer = nullptr;
	wgpu::Buffer transformIndexBuffer = nullptr;
	std::string pipelineName = "NONE";
	PipelineType pipelineType = PipelineType::None;
	std::vector<std::string> passNames = {};
	std::vector<entt::hashed_string> textures = {};
	uint32_t indexCount = 0;
	bool enabled = true;

	Mesh() = default;
	Mesh(ES::Engine::Core &core, const std::vector<glm::vec3> &vertices, const std::vector<glm::vec3> &normals, const std::vector<glm::vec2> &uvs, const std::vector<uint32_t> &indices);

	void Release();
};
}
