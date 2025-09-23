#include "system/EventSystems.hpp"
#include "resource/UIContext.hpp"

void ES::Plugin::Rmlui::System::UpdateMouseMoveEvent(ES::Engine::Core &core)
{
    core.GetResource<ES::Plugin::Rmlui::Resource::UIContext>().UpdateMouseMoveEvent(core);
}
