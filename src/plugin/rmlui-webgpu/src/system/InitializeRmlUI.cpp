#include "InitializeRmlUI.hpp"
#include "utils/WGPURenderInterface.hpp"
#include <RmlUi/Core.h>
#include <RmlUi/Debugger.h>

namespace ES::Plugin::Rmlui::WebGPU::System
{
    void InitializeRmlUI(ES::Engine::Core &core)
    {
        ES::Utils::Log::Info("Initializing RmlUI...");
        
        Rml::SetRenderInterface(new WGPURenderInterface(core));
        Rml::SetSystemInterface(nullptr);
        Rml::SetFileInterface(nullptr);
        
        Rml::Initialise();
        ES::Utils::Log::Info("RmlUI initialized successfully");
        
        Rml::Context* context = Rml::CreateContext("main", Rml::Vector2i(800, 600));
        if (!context) {
            ES::Utils::Log::Error("Failed to create RmlUI context");
            throw std::runtime_error("Failed to create RmlUI context");
        }
        ES::Utils::Log::Info("RmlUI context created successfully");

        if (!Rml::LoadFontFace("assets/fonts/airborne.ttf")) {
            ES::Utils::Log::Error("Failed to load font face");
            throw std::runtime_error("Failed to load font face");
        }
        ES::Utils::Log::Info("Font loaded successfully");
        
        ES::Utils::Log::Info("Loading UI document: assets/ui/user_interface.rml");
        Rml::ElementDocument* document = context->LoadDocument("assets/ui/user_interface.rml");
        if (document) {
            ES::Utils::Log::Info("UI document loaded successfully");
            document->Show();
            ES::Utils::Log::Info("UI document shown");
        } else {
            ES::Utils::Log::Error("Failed to load UI document");
            return;
        }
    }
}

