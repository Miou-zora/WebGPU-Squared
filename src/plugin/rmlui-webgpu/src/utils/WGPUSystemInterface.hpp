#pragma once

#include <RmlUi/Core.h>
#include <chrono>

namespace ES::Plugin::Rmlui::WebGPU::System
{
    class WGPUSystemInterface : public Rml::SystemInterface
    {
    public:
        WGPUSystemInterface() = default;
        ~WGPUSystemInterface() = default;

        double GetElapsedTime() override;
        bool LogMessage(Rml::Log::Type type, const Rml::String& message) override;
    };
}
