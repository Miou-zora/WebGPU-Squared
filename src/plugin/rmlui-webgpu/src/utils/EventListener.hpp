#pragma once

#include <RmlUi/Core.h>
#include <functional>
#include <string>

namespace ES::Engine
{
    class Core;
}

namespace ES::Plugin::Rmlui::Utils
{
    class EventListener : public Rml::EventListener
    {
    public:
        using EventCallback = std::function<void(Rml::Event&)>;
        
        explicit EventListener(Rml::Context& context);
        ~EventListener() override = default;
        
        void ProcessEvent(Rml::Event& event) override;
        
        void SetEventCallback(EventCallback callback);
        void AttachEvents(const std::string& eventType, Rml::Element& element);
        void SetCallback(ES::Engine::Core& core);
        
    private:
        Rml::Context* _context;
        EventCallback _callback;
    };
}
