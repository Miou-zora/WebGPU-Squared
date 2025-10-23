#include "WGPUSystemInterface.hpp"
#include "core/Core.hpp"

namespace ES::Plugin::Rmlui::WebGPU::System
{
    double WGPUSystemInterface::GetElapsedTime()
    {
        static auto start_time = std::chrono::high_resolution_clock::now();
        auto current_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(current_time - start_time);
        return duration.count() / 1000000.0;
    }

    bool WGPUSystemInterface::LogMessage(Rml::Log::Type type, const Rml::String& message)
    {
        switch (type) {
            case Rml::Log::Type::LT_ERROR:
                std::cout << "[ERROR] RmlUI: " << message << std::endl;
                break;
            case Rml::Log::Type::LT_WARNING:
                std::cout << "[WARNING] RmlUI: " << message << std::endl;
                break;
            case Rml::Log::Type::LT_INFO:
                std::cout << "[INFO] RmlUI: " << message << std::endl;
                break;
            case Rml::Log::Type::LT_DEBUG:
                std::cout << "[DEBUG] RmlUI: " << message << std::endl;
                break;
            default:
                std::cout << "[INFO] RmlUI: " << message << std::endl;
                break;
        }
        return true;
    }
}
