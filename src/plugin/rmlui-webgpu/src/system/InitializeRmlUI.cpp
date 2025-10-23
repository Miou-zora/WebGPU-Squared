#include "InitializeRmlUI.hpp"
#include "utils/WGPURenderInterface.hpp"
#include "utils/WGPUSystemInterface.hpp"
#include <RmlUi/Core.h>
#include <RmlUi/Debugger.h>

namespace ES::Plugin::Rmlui::WebGPU::System
{
    void InitializeRmlUI(ES::Engine::Core &core)
    {
        ES::Utils::Log::Debug("Initializing RmlUI...");
        
        auto* renderInterface = new WGPURenderInterface(core);
        auto* systemInterface = new WGPUSystemInterface();
        Rml::SetRenderInterface(renderInterface);
        Rml::SetSystemInterface(systemInterface);
        Rml::SetFileInterface(nullptr);
        
        ES::Utils::Log::Debug("RmlUI: Render interface set: " + std::to_string(reinterpret_cast<uintptr_t>(renderInterface)));
        
        Rml::Initialise();
        ES::Utils::Log::Debug("RmlUI: Initialized");
        
        Rml::Context* context = Rml::CreateContext("main", Rml::Vector2i(800, 600));
        if (!context) {
            ES::Utils::Log::Error("RmlUI: Failed to create context");
            throw std::runtime_error("Failed to create RmlUI context");
        }
        ES::Utils::Log::Debug("RmlUI: Context created");
        
        // Verify the render interface is set on the context
        auto* contextRenderInterface = Rml::GetRenderInterface();
        ES::Utils::Log::Debug("Context render interface: " + std::to_string(reinterpret_cast<uintptr_t>(contextRenderInterface)));
        ES::Utils::Log::Debug("Custom render interface: " + std::to_string(reinterpret_cast<uintptr_t>(renderInterface)));
        ES::Utils::Log::Debug("Render interfaces match: " + std::string(contextRenderInterface == renderInterface ? "true" : "false"));

        if (!Rml::LoadFontFace("assets/fonts/airborne.ttf")) {
            ES::Utils::Log::Error("RmlUI: Failed to load font face");
            throw std::runtime_error("Failed to load font face");
        }
        ES::Utils::Log::Debug("RmlUI: Font loaded");
        
        ES::Utils::Log::Debug("RmlUI: Loading UI document: assets/ui/user_interface.rml");
        
        Rml::ElementDocument* document = context->LoadDocument("assets/ui/user_interface.rml");
        if (document) {
            ES::Utils::Log::Debug("RmlUI: UI document loaded");
            document->Show();
            ES::Utils::Log::Debug("RmlUI: UI document displayed");
            
        } else {
            ES::Utils::Log::Error("RmlUI: Failed to load UI document");
            return;
        }
    }

    void CleanupRmlUI(ES::Engine::Core &core)
    {
        auto* context = Rml::GetContext("main");
        if (context) {
            int documentCount = context->GetNumDocuments();
            for (int i = documentCount - 1; i >= 0; i--) {
                Rml::ElementDocument* doc = context->GetDocument(i);
                if (doc) {
                    doc->Close();
                }
            }
        }
        
        Rml::RemoveContext("main");
        Rml::Shutdown();
        ES::Utils::Log::Debug("RmlUI: Shutdown completed");
    }
}

