#include "PluginRmluiWebgpu.hpp"
#include "system/InitializeRmlUI.hpp"
#include "system/InitializeRmlUIRenderPass.hpp"
#include "system/RenderRmlUI.hpp"
#include "../../webgpu/src/WebGPU.hpp"
#include "RenderingPipeline.hpp"

namespace ES::Plugin::Rmlui::WebGPU
{
    void Plugin::Bind()
    {
        RequirePlugins<ES::Plugin::RenderingPipeline::Plugin>();
        RequirePlugins<ES::Plugin::WebGPU::Plugin>();
        
        RegisterSystems<ES::Plugin::RenderingPipeline::Setup>(
            System::InitializeRmlUI,
            System::InitializeRmlUIRenderPass
        );

        // RmlUI rendering is handled by the render pass system in ToGPU phase
        // No need for additional Draw phase systems
    }
}