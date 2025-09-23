#include "system/UpdateUI.hpp"
#include "resource/UIContext.hpp"

void ES::Plugin::Rmlui::System::Update(ES::Engine::Core &core)
{
    core.GetResource<ES::Plugin::Rmlui::Resource::UIContext>().Update(core);
}

void ES::Plugin::Rmlui::System::Render(ES::Engine::Core &core)
{
    core.GetResource<ES::Plugin::Rmlui::Resource::UIContext>().Render(core);
}
