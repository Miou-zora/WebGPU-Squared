#pragma once

#include "APlugin.hpp"

namespace ES::Plugin::ImGUI {
    namespace WebGPU {
        class Plugin : public ES::Engine::APlugin {
          public:
            using APlugin::APlugin;
            ~Plugin() = default;

            void Bind() final;
        };
    }
}
