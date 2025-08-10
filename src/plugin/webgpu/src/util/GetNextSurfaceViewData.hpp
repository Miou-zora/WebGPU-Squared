#pragma once

#include "webgpu.hpp"

namespace ES::Plugin::WebGPU::Util {

wgpu::TextureView GetNextSurfaceViewData(wgpu::Surface &surface);

}
