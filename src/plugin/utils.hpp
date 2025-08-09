#pragma once

#include <fstream>
#include <sstream>
#include <string_view>

#include "webgpu.hpp"

uint32_t ceilToNextMultiple(uint32_t value, uint32_t step);

std::string_view toStdStringView(wgpu::StringView wgpuStringView);

std::string loadFile(const std::string &filePath);
