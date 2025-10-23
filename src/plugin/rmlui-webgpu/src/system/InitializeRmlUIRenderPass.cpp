#include "InitializeRmlUIRenderPass.hpp"
#include "utils/WGPURenderInterface.hpp"
#include "../../webgpu/src/WebGPU.hpp"
#include <RmlUi/Core.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include "resource/Window/Window.hpp"

namespace ES::Plugin::Rmlui::WebGPU::System
{
    void InitializeRmlUIRenderPass(ES::Engine::Core &core)
    {
        auto &renderGraph = core.GetResource<RenderGraph>();
        
        renderGraph.AddRenderPass(RenderPassData{
            .name = "RmlUI",
            .shaderName = "RmlUI",
            .pipelineType = PipelineType::_2D,
            .loadOp = wgpu::LoadOp::Load,
            .outputColorTextureName = {"WindowColorTexture"},
            .bindGroups = {
                {.groupIndex = 0,
                 .type = BindGroupsLinks::AssetType::BindGroup,
                 .name = "RmlUIUniforms"}
            },
           .uniqueRenderCallback = [](wgpu::RenderPassEncoder &renderPass, ES::Engine::Core &core) {
               auto* context = Rml::GetContext("main");
               if (!context) {
                   ES::Utils::Log::Error("RmlUI context not found in render pass callback");
                   return;
               }
               
               auto* renderInterface = static_cast<WGPURenderInterface*>(Rml::GetRenderInterface());
               if (!renderInterface) {
                   ES::Utils::Log::Error("RmlUI render interface not found in render pass callback");
                   return;
               }
               
                // Set up orthographic projection matrix for UI
                auto &window = core.GetResource<ES::Plugin::Window::Resource::Window>();
                int width, height;
                glfwGetFramebufferSize(window.GetGLFWWindow(), &width, &height);
                
                // Set context dimensions to match window
                context->SetDimensions(Rml::Vector2i(width, height));
                
                // Set up orthographic projection matrix
                glm::mat4 orthoMatrix = glm::ortho(0.0f, (float)width, (float)height, 0.0f, -1.0f, 1.0f);
                
                Rml::Matrix4f rmlMatrix;
                for (int i = 0; i < 4; ++i) {
                    for (int j = 0; j < 4; ++j) {
                        rmlMatrix[i][j] = orthoMatrix[i][j];
                    }
                }
                renderInterface->SetTransform(&rmlMatrix);
                
               // Update the context to process layout and animations
               context->Update();
                
                // Set the render pass encoder before calling context->Render()
                // because Render() is what will call our CompileGeometry/RenderGeometry methods
                renderInterface->SetRenderPassEncoder(renderPass);
                
                // Render the UI - this generates and queues geometry
                context->Render();
                
                // Render all queued geometry
                renderInterface->RenderQueuedGeometry();
            }
        });
    }
}
