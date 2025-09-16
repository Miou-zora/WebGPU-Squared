#pragma once

#include <RmlUi/Core.h>

#include "utils/EventListener.hpp"

#include "AUIContext.hpp"

namespace ES::Plugin::Rmlui::Resource
{
    namespace
    {
        struct TransparentHash
        {
            using is_transparent = void;

            std::size_t operator()(std::string_view key) const noexcept { return std::hash<std::string_view>{}(key); }
        };

        struct TransparentEqual
        {
            using is_transparent = void;

            bool operator()(std::string_view lhs, std::string_view rhs) const noexcept { return lhs == rhs; }
        };
    }

    class UIContext : public AUIContext
    {
    public:
        UIContext() = default;
        ~UIContext() = default;

        UIContext(const UIContext &) = delete;
        UIContext &operator=(const UIContext &) = delete;
        UIContext(UIContext &&) noexcept = default;
        UIContext &operator=(UIContext &&) noexcept = default;

        void Update(ES::Engine::Core &core) override;
        void Render(ES::Engine::Core &core) override;
        void Destroy(ES::Engine::Core &core) override;
        void UpdateMouseMoveEvent(ES::Engine::Core &core) override;
        void BindEventCallback(ES::Engine::Core &core) override;
        void SetFont(const std::string &fontPath) override;
        void InitDocument(const std::string &docPath) override;
        const std::string &GetTitle() const override;
        void UpdateInnerContent(const std::string &childId, const std::string &content) override;
        void SetTransformProperty(const std::string &childId, const std::vector<TransformParam> &transforms) override;

    protected:
        void __setup(ES::Engine::Core &core) override;

    private:
        std::unique_ptr<Rml::Context> _context;
        std::unique_ptr<Rml::ElementDocument> _document;
        std::unordered_map<std::string, std::unique_ptr<ES::Plugin::UI::Utils::EventListener>, TransparentHash, TransparentEqual> _events;

    private:
        bool _isReady() const;
    };
}
