#include "UIContext.hpp"
#include "resource/Window/Window.hpp"

namespace ES::Plugin::Rmlui::Resource
{
    void UIContext::__setup(ES::Engine::Core &core)
    {
        Rml::SetSystemInterface(_systemInterface.get());
        Rml::SetRenderInterface(_renderInterface.get());
        Rml::Initialise();

        const auto &windowSize = core.GetResource<ES::Plugin::Window::Resource::Window>().GetSize();
        auto context = Rml::CreateContext("main", Rml::Vector2i(windowSize.x, windowSize.y));
        if (!context)
        {
            Destroy(core);
            throw std::runtime_error("Failed to create Rml::Context");
        }
        _context->SetDimensions(Rml::Vector2i(windowSize.x, windowSize.y));
    }

    void UIContext::Destroy(ES::Engine::Core &core)
    {
        if (_document != nullptr)
            _document->Close();
        if (_context != nullptr)
            Rml::RemoveContext(_context->GetName());
        _events.clear();
        Rml::Shutdown();
    }

    void UIContext::Update(ES::Engine::Core &core) {}
    void UIContext::Render(ES::Engine::Core &core) {}
    void UIContext::UpdateMouseMoveEvent(ES::Engine::Core &core) {}

}
