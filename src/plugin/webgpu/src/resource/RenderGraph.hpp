#pragma once

#include "structs.hpp"
#include "Entity.hpp"

class RenderGraph {
    public:
        RenderGraph() = default;
        ~RenderGraph() = default;

        void AddRenderPass(const RenderPassData& passData) {
            nodes.push_back(passData);
        }

        void Execute(ES::Engine::Core &core) {
            for (const auto& pass : nodes) {
                executePass(pass, core);
            }
        }

    private:
        void executePass(const RenderPassData& renderPassData, ES::Engine::Core &core) {
            wgpu::Queue &queue = core.GetResource<wgpu::Queue>();
            wgpu::Device &device = core.GetResource<wgpu::Device>();

            wgpu::CommandEncoderDescriptor encoderDesc(wgpu::Default);
            std::string encoderDescLabel = fmt::format("CreateRenderPass::{}::CommandEncoder", renderPassData.name);
            encoderDesc.label = wgpu::StringView(encoderDescLabel);
            auto commandEncoder = device.createCommandEncoder(encoderDesc);
            if (commandEncoder == nullptr) throw std::runtime_error(fmt::format("CreateRenderPass::{}::Command encoder is not created, cannot draw sprite.", renderPassData.name));

            wgpu::RenderPassDescriptor renderPassDesc(wgpu::Default);
            std::string renderPassDescLabel = fmt::format("CreateRenderPass::{}::RenderPass", renderPassData.name);
            renderPassDesc.label = wgpu::StringView(renderPassDescLabel);

            // Target Texture
            wgpu::RenderPassColorAttachment renderPassColorAttachment(wgpu::Default);
            {
                renderPassColorAttachment.view = core.GetResource<TextureManager>().Get(entt::hashed_string(renderPassData.outputColorTextureName.c_str())).textureView;
                renderPassColorAttachment.loadOp = renderPassData.loadOp;
                renderPassColorAttachment.storeOp = wgpu::StoreOp::Store;
                if (renderPassData.clearColor.has_value()) {
                    glm::vec4 clearColor = renderPassData.clearColor.value()(core);
                    renderPassColorAttachment.clearValue = wgpu::Color(
                        clearColor.r,
                        clearColor.g,
                        clearColor.b,
                        clearColor.a
                    );
                }
            }

            renderPassDesc.colorAttachmentCount = 1;
            renderPassDesc.colorAttachments = &renderPassColorAttachment;

            wgpu::RenderPassDepthStencilAttachment depthStencilAttachment(wgpu::Default);
            {
                depthStencilAttachment.view = core.GetResource<TextureManager>().Get(entt::hashed_string(renderPassData.outputDepthTextureName.c_str())).textureView;
                depthStencilAttachment.depthClearValue = 1.0f;
                depthStencilAttachment.depthLoadOp = renderPassData.loadOp;
                depthStencilAttachment.depthStoreOp = wgpu::StoreOp::Store;
            }

            renderPassDesc.depthStencilAttachment = &depthStencilAttachment;

            wgpu::RenderPassEncoder renderPass = commandEncoder.beginRenderPass(renderPassDesc);

            // Select which render pipeline to use
            if (renderPassData.pipelineName.has_value()) {
                PipelineData &pipelineData = core.GetResource<Pipelines>().renderPipelines[renderPassData.pipelineName.value()];
                renderPass.setPipeline(pipelineData.pipeline);
            }

            for (const BindGroupsLinks &link : renderPassData.bindGroups) {
                auto name = link.name;
                if (link.type == BindGroupsLinks::AssetType::BindGroup) {
                    auto &bindGroups = core.GetResource<BindGroups>();
                    if (bindGroups.groups.contains(name)) {
                        renderPass.setBindGroup(link.groupIndex, bindGroups.groups[name], 0, nullptr);
                    } else {
                        ES::Utils::Log::Error(fmt::format("CreateRenderPass::{}: Bind group with name '{}' not found.", renderPassData.name, name));
                    }
                } else if (link.type == BindGroupsLinks::AssetType::TextureView) {
                    auto &textures = core.GetResource<TextureManager>();
                    auto textureID = entt::hashed_string(name.c_str());
                    if (textures.Contains(textureID)) {
                        auto &texture = textures.Get(textureID);
                        renderPass.setBindGroup(link.groupIndex, texture.bindGroup, 0, nullptr);
                    } else {
                        ES::Utils::Log::Error(fmt::format("CreateRenderPass::{}: Texture with name '{}' not found.", renderPassData.name, name));
                    }
                } else {
                    ES::Utils::Log::Error(fmt::format("CreateRenderPass::{}: Unknown BindGroupsLinks type.", renderPassData.name));
                }
            }

            if (renderPassData.uniqueRenderCallback.has_value()) {
                renderPassData.uniqueRenderCallback.value()(renderPass, core);
            } else {
                core.GetRegistry().view<ES::Plugin::WebGPU::Component::Mesh, ES::Plugin::Object::Component::Transform>().each([&](auto e, ES::Plugin::WebGPU::Component::Mesh &mesh, ES::Plugin::Object::Component::Transform &transform) {
                    if (!mesh.enabled || mesh.pipelineName != renderPassData.pipelineName) return;

                    const auto &transformMatrix = transform.getTransformationMatrix();
                    queue.writeBuffer(uniformBuffer, offsetof(MyUniforms, modelMatrix), &transformMatrix, sizeof(MyUniforms::modelMatrix));

                    if (renderPassData.perEntityCallback.has_value()) {
                        renderPassData.perEntityCallback.value()(renderPass, core, mesh, transform, ES::Engine::Entity(e));
                    }

                    renderPass.setVertexBuffer(0, mesh.pointBuffer, 0, mesh.pointBuffer.getSize());
                    renderPass.setIndexBuffer(mesh.indexBuffer, wgpu::IndexFormat::Uint32, 0, mesh.indexBuffer.getSize());

                    renderPass.drawIndexed(mesh.indexCount, 1, 0, 0, 0);
                });
            }


            renderPass.end();
            renderPass.release();

            // Finally encode and submit the render pass
            wgpu::CommandBufferDescriptor cmdBufferDescriptor(wgpu::Default);
            cmdBufferDescriptor.label = wgpu::StringView(fmt::format("CreateRenderPass::{}::CommandBuffer", renderPassData.name));
            auto command = commandEncoder.finish(cmdBufferDescriptor);
            commandEncoder.release();

            queue.submit(1, &command);
            command.release();
        }

        // For now I will consider renderpass as non dependent for simplicity
        std::list<RenderPassData> nodes;
};
