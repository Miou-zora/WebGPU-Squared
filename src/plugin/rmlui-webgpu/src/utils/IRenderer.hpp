#pragma once

#include <RmlUi/Core.h>

namespace ES::Plugin::Rmlui
{
    class IRenderer : public Rml::RenderInterface
    {
    public:
        virtual void BeginFrame() = 0;
        virtual void EndFrame() = 0;
    };
}
