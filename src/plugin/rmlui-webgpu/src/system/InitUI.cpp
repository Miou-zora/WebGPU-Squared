#include "InitUI.hpp"
#include "resource/UIContext.hpp"

namespace ES::Plugin::Rmlui::WebGPU::System
{
    void Init(ES::Engine::Core &core)
    {
        // core.GetResource<ES::Plugin::Rmlui::WebGPU::Resource::UIContext>().Init<
        //     ES::Plugin::Rmlui::WebGPU::System::SystemInterface,
        //     ES::Plugin::Rmlui::WebGPU::System::RenderInterface>(core);
    }

    void Destroy(ES::Engine::Core &core)
    {
        core.GetResource<ES::Plugin::Rmlui::Resource::UIContext>().Destroy(core);
    }
}
