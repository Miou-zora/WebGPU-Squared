#include "CreateSprite.hpp"

namespace ES::Plugin::WebGPU::Util {

void CreateSprite(const glm::vec2 &position, const glm::vec2 &size, std::vector<glm::vec3> &vertices, std::vector<glm::vec3> &normals, std::vector<glm::vec2> &texCoords, std::vector<uint32_t> &indices)
{
	vertices.resize(4);
	vertices[0] = glm::vec3(position.x, position.y, 0.0f);
	vertices[1] = glm::vec3(position.x + size.x, position.y, 0.0f);
	vertices[2] = glm::vec3(position.x + size.x, position.y + size.y, 0.0f);
	vertices[3] = glm::vec3(position.x, position.y + size.y, 0.0f);

	normals.resize(4);
	normals[0] = glm::vec3(0.0f, 0.0f, 1.0f);
	normals[1] = glm::vec3(0.0f, 0.0f, 1.0f);
	normals[2] = glm::vec3(0.0f, 0.0f, 1.0f);
	normals[3] = glm::vec3(0.0f, 0.0f, 1.0f);

	texCoords.resize(4);
	texCoords[0] = glm::vec2(0.0f, 0.0f);
	texCoords[1] = glm::vec2(1.0f, 0.0f);
	texCoords[2] = glm::vec2(1.0f, 1.0f);
	texCoords[3] = glm::vec2(0.0f, 1.0f);

	indices.resize(6);
	indices[0] = 0; // Bottom left
	indices[1] = 1; // Bottom right
	indices[2] = 2; // Top right
	indices[3] = 0; // Bottom left
	indices[4] = 2; // Top right
	indices[5] = 3; // Top left
}
}
