#pragma once

#include "webgpu.hpp"
#include "Core.hpp"

namespace ES::Plugin::WebGPU::Util {

void GetNextSurfaceViewData(ES::Engine::Core &core, wgpu::Surface &surface);

}
