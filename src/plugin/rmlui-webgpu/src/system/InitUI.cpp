#include "InitUI.hpp"
#include "resource/UIContext.hpp"
#include "utils/SystemInterface.hpp"
#include "utils/WGPURenderInterface.hpp"

namespace ES::Plugin::Rmlui::WebGPU::System
{
    void Init(ES::Engine::Core &core)
    {
        core.GetResource<ES::Plugin::Rmlui::Resource::UIContext>().Init<
            ES::Plugin::UI::Utils::SystemInterface,
            ES::Plugin::Rmlui::WebGPU::System::WGPURenderInterface
        >(core);
    }

    void Destroy(ES::Engine::Core &core)
    {
        core.GetResource<ES::Plugin::Rmlui::Resource::UIContext>().Destroy(core);
    }
}
