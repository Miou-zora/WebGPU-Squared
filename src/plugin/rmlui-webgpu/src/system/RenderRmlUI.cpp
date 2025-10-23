#include "RenderRmlUI.hpp"
#include "utils/WGPURenderInterface.hpp"
#include "../../webgpu/src/WebGPU.hpp"
#include <RmlUi/Core.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include "resource/Window/Window.hpp"

namespace ES::Plugin::Rmlui::WebGPU::System
{
    void RenderRmlUI(ES::Engine::Core &core)
    {
        auto* context = Rml::GetContext("main");
        if (!context) {
            ES::Utils::Log::Error("RmlUI context not found during render");
            return;
        }
        
        auto* renderInterface = static_cast<WGPURenderInterface*>(Rml::GetRenderInterface());
        if (!renderInterface) {
            ES::Utils::Log::Error("RmlUI render interface not found during render");
            return;
        }
        
        auto &window = core.GetResource<ES::Plugin::Window::Resource::Window>();
        int width, height;
        glfwGetFramebufferSize(window.GetGLFWWindow(), &width, &height);
        
        glm::mat4 orthoMatrix = glm::ortho(0.0f, (float)width, (float)height, 0.0f, -1.0f, 1.0f);
        
        Rml::Matrix4f rmlMatrix;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                rmlMatrix[i][j] = orthoMatrix[i][j];
            }
        }
        renderInterface->SetTransform(&rmlMatrix);
        context->Render();
    }
}
