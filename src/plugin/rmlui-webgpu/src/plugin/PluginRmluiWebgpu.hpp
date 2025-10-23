#pragma once

#include "plugin/APlugin.hpp"

namespace ES::Plugin::Rmlui::WebGPU
{
    class Plugin : public ES::Engine::APlugin {
        public:
            using APlugin::APlugin;
            ~Plugin() = default;

            void Bind() final;
    };
}