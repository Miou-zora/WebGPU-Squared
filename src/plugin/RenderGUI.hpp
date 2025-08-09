#pragma once

#include "webgpu.hpp"
#include "Core.hpp"

//TODO: use ImGUI namespace
namespace ES::Plugin::ImGUI::WebGPU::System {

void RenderGUI(wgpu::RenderPassEncoder renderPass, ES::Engine::Core &core);

}
