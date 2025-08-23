#include "Render.hpp"
#include "WebGPU.hpp"
#include "structs.hpp"

namespace ES::Plugin::WebGPU::System {

void Render(ES::Engine::Core &core)
{
    wgpu::Surface &surface = core.GetResource<wgpu::Surface>();

    // Stop having access to the texture view after the render pass is done
    if (!core.GetResource<TextureManager>().Contains("WindowColorTexture")) {
        throw std::runtime_error("No WindowColorTexture found in texture manager");
    }
    core.GetResource<TextureManager>().Get("WindowColorTexture").textureView.release();
    // Present the rendered texture to the surface
    surface.present();
}
}
