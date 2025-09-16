#pragma once

#include "Engine.hpp"
#include "utils/IRenderer.hpp"

template <typename T>
concept CSystemInterface = std::is_base_of_v<Rml::SystemInterface, T>;

template <typename T>
concept CRenderInterface = std::is_base_of_v<Rml::RenderInterface, T> &&
                           requires(ES::Engine::Core &a) {
                               T(a);
                           };

namespace ES::Plugin::Rmlui
{
    namespace
    {
        enum class TransformType
        {
            Rotate,
            TranslateX,
            TranslateY,
        };

        struct TransformParam
        {
            TransformType type;
            float value;
        };
    }
    class AUIContext
    {
    public:
        template <CSystemInterface TSystemInterface, CRenderInterface TRenderInterface>
        void Init(ES::Engine::Core &core)
        {
            _systemInterface = std::make_unique<TSystemInterface>();
            _renderInterface = std::make_unique<TRenderInterface>(core);
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
        virtual void BindEventCallback(ES::Engine::Core &core) = 0;
        virtual void SetFont(const std::string &fontPath) = 0;
        virtual void InitDocument(const std::string &docPath) = 0;
        virtual const std::string &GetTitle() const = 0;
        virtual void UpdateInnerContent(const std::string &childId, const std::string &content) = 0;
        virtual void SetTransformProperty(const std::string &childId, const std::vector<TransformParam> &transforms) = 0;

    protected:
        std::unique_ptr<Rml::SystemInterface> _systemInterface;
        std::unique_ptr<IRenderer> _renderInterface;

        virtual void __setup(ES::Engine::Core &core) = 0;
    };
}
