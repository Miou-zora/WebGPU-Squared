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
        void AttachEventHandlers(const std::string &elementId, const std::string &eventType, ES::Plugin::Rmlui::Utils::EventListener::EventCallback callback) override;
        void DetachEventHandler(const std::string &elementId, const std::string &eventType) override;
        std::string GetValue(const std::string &elementId) const override;
        std::string GetStyle(const std::string &elementId, const std::string &property) const override;
        bool SetStyleProperty(const std::string &elementId, const std::string &property, const std::string &value) const override;

    protected:
        void __setup(ES::Engine::Core &core) override;

    private:
        // TODO: find a way to check if raw ptr are correctly deleted
        Rml::Context *_context = nullptr;
        Rml::ElementDocument *_document = nullptr;
        std::unordered_map<std::string, std::unique_ptr<ES::Plugin::Rmlui::Utils::EventListener>, TransparentHash, TransparentEqual> _events;

    private:
        bool _isReady() const;
    };
}
