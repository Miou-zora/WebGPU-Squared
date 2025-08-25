#include "webgpu.hpp"
#include "UpdateLights.hpp"
#include "structs.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

namespace ES::Plugin::WebGPU::Util {

void UpdateLights(ES::Engine::Core &core)
{
    auto &device = core.GetResource<wgpu::Device>();
    auto &pipelineData = core.GetResource<Pipelines>().renderPipelines["Lighting"];
    auto &bindGroups = core.GetResource<BindGroups>();
    auto &queue = core.GetResource<wgpu::Queue>();
    auto &lights = core.GetResource<std::vector<Light>>();

    lightsBuffer.destroy();
    lightsBuffer.release();

    wgpu::BufferDescriptor lightsBufferDesc(wgpu::Default);
    lightsBufferDesc.size = sizeof(Light) * std::max(lights.size(), size_t(1)) + sizeof(uint32_t) + 12 /* (padding) */;
    lightsBufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage;
    lightsBufferDesc.label = wgpu::StringView("Lights Buffer");
    lightsBuffer = device.createBuffer(lightsBufferDesc);

    uint32_t lightsCount = static_cast<uint32_t>(lights.size());
    queue.writeBuffer(lightsBuffer, 0, &lightsCount, sizeof(uint32_t));
    queue.writeBuffer(lightsBuffer, sizeof(uint32_t), lights.data(), sizeof(Light) * lights.size());

    bindGroups.groups["2"].release();

    wgpu::BindGroupEntry bindingLights(wgpu::Default);
    bindingLights.binding = 0;
    bindingLights.buffer = lightsBuffer;
    bindingLights.size = sizeof(Light) * std::max(lights.size(), size_t(1)) + sizeof(uint32_t) + 12 /* (padding) */; // TODO: Resize when adding a new light

    std::array<wgpu::BindGroupEntry, 1> lightsBindings = { bindingLights };

    wgpu::BindGroupDescriptor bindGroupLightsDesc(wgpu::Default);
    bindGroupLightsDesc.layout = pipelineData.bindGroupLayouts[1];
    bindGroupLightsDesc.entryCount = lightsBindings.size();
    bindGroupLightsDesc.entries = lightsBindings.data();
    bindGroupLightsDesc.label = wgpu::StringView("Lights Bind Group");
    auto bg = device.createBindGroup(bindGroupLightsDesc);

    bindGroups.groups["2"] = bg;

    for (auto &additionalLight : additionalDirectionalLights) {
        additionalLight.bindGroup.release();
        additionalLight.buffer.release();
    }


    additionalDirectionalLights.clear();


    uint32_t lightIndex = 0;
    for (auto &light : lights) {
        if (light.type == Light::Type::Directional) {
            AdditionalDirectionalLight additionalDataLight;
            
            glm::vec3 lightDirection = glm::normalize(light.direction);
            glm::vec3 posOfLight = -lightDirection * 50.0f;
            
            glm::mat4 lightProjection = glm::ortho(-50.0f, 50.0f, -50.0f, 50.0f, 0.1f, 100.0f);
            
            glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f);
            glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
            
            if (glm::abs(glm::dot(lightDirection, up)) > 0.9f) {
                up = glm::vec3(1.0f, 0.0f, 0.0f);
            }
            
            glm::mat4 lightView = glm::lookAt(posOfLight, target, up);
            glm::mat4 lightSpaceMatrix = lightProjection * lightView;
            additionalDataLight.lightViewProj = lightSpaceMatrix;
            light.lightViewProjMatrix = lightSpaceMatrix;

            wgpu::BufferDescriptor bufferDesc(wgpu::Default);
            bufferDesc.size = sizeof(glm::mat4);
            bufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform;
            bufferDesc.label = wgpu::StringView("Additional Directional Light Buffer");
            additionalDataLight.buffer = device.createBuffer(bufferDesc);

            queue.writeBuffer(additionalDataLight.buffer, 0, &additionalDataLight.lightViewProj, sizeof(glm::mat4));

            wgpu::BindGroupEntry bindingAdditionalDirectionalLights(wgpu::Default);
            bindingAdditionalDirectionalLights.binding = 0;
            bindingAdditionalDirectionalLights.buffer = additionalDataLight.buffer;
            bindingAdditionalDirectionalLights.size = sizeof(glm::mat4);

            std::array<wgpu::BindGroupEntry, 1> additionalDirectionalLightsBindings = { bindingAdditionalDirectionalLights };

            wgpu::BindGroupDescriptor bindGroupAdditionalDirectionalLightsDesc(wgpu::Default);
            bindGroupAdditionalDirectionalLightsDesc.layout = core.GetResource<Pipelines>().renderPipelines["ShadowPass"].bindGroupLayouts[1];
            bindGroupAdditionalDirectionalLightsDesc.entryCount = additionalDirectionalLightsBindings.size();
            bindGroupAdditionalDirectionalLightsDesc.entries = additionalDirectionalLightsBindings.data();
            bindGroupAdditionalDirectionalLightsDesc.label = wgpu::StringView("Additional Data Lights Bind Group");
            additionalDataLight.bindGroup = device.createBindGroup(bindGroupAdditionalDirectionalLightsDesc);

            additionalDirectionalLights.push_back(additionalDataLight);

            light.lightIndex = lightIndex;
            lightIndex++;
        }
    }
}
}
