#include "Render.hpp"
#include "webgpu.hpp"
#include "structs.hpp"

namespace ES::Plugin::WebGPU::System {

void Render(ES::Engine::Core &core)
{
    wgpu::Surface &surface = core.GetResource<wgpu::Surface>();

    // Stop having access to the texture view after the render pass is done
    textureView.release();
    // Present the rendered texture to the surface
    surface.present();
}
}
