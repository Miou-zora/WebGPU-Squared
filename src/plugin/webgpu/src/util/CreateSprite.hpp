#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace ES::Plugin::WebGPU::Util {

void CreateSprite(const glm::vec2 &position, const glm::vec2 &size, std::vector<glm::vec3> &vertices, std::vector<glm::vec3> &normals, std::vector<glm::vec2> &texCoords, std::vector<uint32_t> &indices);

}

