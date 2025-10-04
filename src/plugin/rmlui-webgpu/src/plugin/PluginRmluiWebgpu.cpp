#include "RmluiWebgpu.hpp"
#include "RenderingPipeline.hpp"

namespace ES::Plugin::Rmlui {
    void Plugin::Bind() {
        RequirePlugins<ES::Plugin::RenderingPipeline::Plugin>();

        RegisterResource(Resource::UIContext());

        RegisterSystems<ES::Plugin::RenderingPipeline::RenderSetup>(ES::Plugin::Rmlui::System::UpdateMouseMoveEvent,
                                                                    ES::Plugin::Rmlui::System::Update);
        RegisterSystems<ES::Plugin::RenderingPipeline::ToGPU>(ES::Plugin::Rmlui::System::Render);
        RegisterSystems<ES::Plugin::RenderingPipeline::Init>(ES::Plugin::Rmlui::WebGPU::System::Init);
        RegisterSystems<ES::Engine::Scheduler::Shutdown>(ES::Plugin::Rmlui::WebGPU::System::Destroy);
    }
}
