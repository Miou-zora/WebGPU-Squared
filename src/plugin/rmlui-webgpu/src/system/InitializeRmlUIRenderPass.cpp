#include "InitializeRmlUIRenderPass.hpp"
#include "utils/WGPURenderInterface.hpp"
#include "../../webgpu/src/WebGPU.hpp"
#include <RmlUi/Core.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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
                if (!context) return;
                
                auto* renderInterface = static_cast<WGPURenderInterface*>(Rml::GetRenderInterface());
                if (!renderInterface) return;
                
                renderInterface->SetRenderPassEncoder(renderPass);
                context->Render();
                renderInterface->RenderQueuedGeometry();
            }
        });
    }
}
