#pragma once

#include "Engine.hpp"

namespace ES::Plugin::Rmlui
{
    class AUIContext
    {
    public:
        template <typename TSystemInterface, typename TRenderInterface>
        void Init(ES::Engine::Core &core)
        {
            _system_interface = std::make_unique<TSystemInterface>();
            _render_interface = std::make_unique<TRenderInterface>();
            this->__setup(core);
        }
        virtual ~AUIContext() = default;
        AUIContext() = default;

        AUIContext(const AUIContext &) = delete;
        AUIContext &operator=(const AUIContext &) = delete;
        AUIContext(AUIContext &&) noexcept = default;
        AUIContext &operator=(AUIContext &&) noexcept = default;

        virtual void Update(ES::Engine::Core &core) = 0;
        virtual void Render(ES::Engine::Core &core) = 0;
        virtual void Destroy(ES::Engine::Core &core) = 0;
        virtual void UpdateMouseMoveEvent(ES::Engine::Core &core) = 0;

    protected:
        std::unique_ptr<Rml::SystemInterface> _system_interface;
        std::unique_ptr<Rml::RenderInterface> _render_interface;

        virtual void __setup(ES::Engine::Core &core) = 0;
    };
}
