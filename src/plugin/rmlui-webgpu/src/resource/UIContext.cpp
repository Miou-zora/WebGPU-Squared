#include "UIContext.hpp"
#include "resource/Window/Window.hpp"

namespace ES::Plugin::Rmlui::Resource
{
    void UIContext::__setup(ES::Engine::Core &core)
    {
        Rml::SetSystemInterface(_systemInterface.get());
        Rml::SetRenderInterface(_renderInterface.get());
        Rml::Initialise();

        const auto &windowSize = core.GetResource<ES::Plugin::Window::Resource::Window>().GetSize();
        auto context = Rml::CreateContext("main", Rml::Vector2i(windowSize.x, windowSize.y));
        if (!context)
        {
            Destroy(core);
            throw std::runtime_error("Failed to create Rml::Context");
        }
        _context->SetDimensions(Rml::Vector2i(windowSize.x, windowSize.y));
    }

    void UIContext::Destroy(ES::Engine::Core &core)
    {
        if (_document != nullptr)
            _document->Close();
        if (_context != nullptr)
            Rml::RemoveContext(_context->GetName());
        _events.clear();
        Rml::Shutdown();
    }

    void UIContext::Update(ES::Engine::Core &core)
    {
        if (!_isReady())
            return;

        const auto &windowSize = core.GetResource<ES::Plugin::Window::Resource::Window>().GetSize();

        _context->SetDimensions(Rml::Vector2i(windowSize.x, windowSize.y));
        _context->Update();
    }
    void UIContext::Render(ES::Engine::Core &core)
    {
        if (!_isReady())
            return;

        _renderInterface.get()->BeginFrame();
        _context->Render();
        _renderInterface.get()->EndFrame();
    }
    void UIContext::BindEventCallback(ES::Engine::Core &core)
    {
        const std::string key = "rmlui::mousecallback";

        auto &listener = _events[key];
        if (!listener)
        {
            listener = std::make_unique<ES::Plugin::UI::Utils::EventListener>(*_context);
            listener->SetCallback(core);
        }
    }

    void UIContext::UpdateMouseMoveEvent(ES::Engine::Core &core)
    {
        const auto &window = core.GetResource<ES::Plugin::Window::Resource::Window>().GetGLFWWindow();
        double x;
        double y;

        glfwGetCursorPos(window, &x, &y);
        _context->ProcessMouseMove(static_cast<int>(x), static_cast<int>(y), 0);
    }

    bool UIContext::_isReady() const
    {
        return (
            _context != nullptr &&
            _document != nullptr &&
            _systemInterface != nullptr &&
            _renderInterface != nullptr);
    }

    void UIContext::SetFont(const std::string &fontPath)
    {
        if (_context)
        {
            if (!Rml::LoadFontFace(fontPath))
            {
                ES::Utils::Log::Error(fmt::format("Rmlui could not load the font {}", fontPath));
            }
        }
        else
        {
            ES::Utils::Log::Warn("Rmlui font can not be assign as it has not been initialized");
        }
    }

    void UIContext::InitDocument(const std::string &docPath)
    {
        if (!_context)
            return;

        _context->UnloadAllDocuments();
        _document.reset(_context->LoadDocument(docPath));
        if (!_document)
            throw std::runtime_error("Rmlui did not succeed reading document");

        _document->Show();
        _document->SetProperty("width", "100%");
        _document->SetProperty("height", "100%");
    }

    const std::string &UIContext::GetTitle() const
    {
        if (!_context || !_document)
        {
            static const std::string empty;
            return empty;
        }
        return _document->GetTitle();
    }

    void UIContext::UpdateInnerContent(const std::string &childId, const std::string &content)
    {
        if (!_isReady())
        {
            ES::Utils::Log::Error(fmt::format("RmlUi: Could not update inner content on {}: No active document", childId));
            return;
        }

        Rml::Element *element = _document->GetElementById(childId.c_str());
        if (element)
        {
            if (element->GetInnerRML() != content)
                element->SetInnerRML(content);
        }
        else
        {
            ES::Utils::Log::Warn(
                fmt::format("RmlUi: Could not update node id '{}' with '{}': Not found", childId, content));
        }
    }

    void UIContext::SetTransformProperty(const std::string &childId, const std::vector<TransformParam> &transforms)
    {
        if (!_isReady())
        {
            ES::Utils::Log::Error(
                fmt::format("RmlUi: Could not set transform property on {}: No active document", childId));
            return;
        }

        std::vector<Rml::TransformPrimitive> rmlTransforms;

        for (const auto &t : transforms)
        {
            switch (t.type)
            {
                using enum TransformType;

            case Rotate:
                rmlTransforms.push_back(Rml::Transforms::Rotate2D{t.value});
                break;
            case TranslateX:
                rmlTransforms.push_back(Rml::Transforms::TranslateX{t.value});
                break;
            case TranslateY:
                rmlTransforms.push_back(Rml::Transforms::TranslateY{t.value});
                break;
            }
        }

        auto property = Rml::Transform::MakeProperty(rmlTransforms);

        Rml::Element *element = _document->GetElementById(childId.c_str());
        if (element)
            element->SetProperty(Rml::PropertyId::Transform, property);
        else
            ES::Utils::Log::Warn(fmt::format("RmlUi: Could not apply property to node id '{}': Not found", childId));
    }
}
