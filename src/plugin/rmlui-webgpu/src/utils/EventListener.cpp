#include "EventListener.hpp"
#include "core/Core.hpp"

namespace ES::Plugin::Rmlui::Utils
{
    EventListener::EventListener(Rml::Context& context) : _context(&context), _callback(nullptr)
    {
    }
    
    void EventListener::ProcessEvent(Rml::Event& event)
    {
        if (_callback)
        {
            _callback(event);
        }
    }
    
    void EventListener::SetEventCallback(EventCallback callback)
    {
        _callback = callback;
    }
    
    void EventListener::AttachEvents(const std::string& eventType, Rml::Element& element)
    {
        element.AddEventListener(eventType, this);
    }
    
    void EventListener::SetCallback(ES::Engine::Core& core)
    {
        // This method can be used to set up core-specific callbacks if needed
        // For now, it's a placeholder
    }
}
