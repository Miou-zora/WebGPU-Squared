#pragma once

#include "util/webgpu.hpp"
#include "Core.hpp"

//TODO: use ImGUI namespace
namespace ES::Plugin::ImGUI::WebGPU::Util {

void RenderGUI(wgpu::RenderPassEncoder renderPass, ES::Engine::Core &core);

}
