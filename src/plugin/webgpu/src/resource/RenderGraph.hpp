#pragma once

#include "structs.hpp"
#include "entity/Entity.hpp"

// TODO: Add namespace
class RenderGraph {
    public:
        RenderGraph() = default;
        ~RenderGraph() = default;

        void AddRenderPass(const RenderPassData& passData) {
            order.push_back({ "RenderPassData", singleRenderPasses.size() });
            singleRenderPasses.push_back(passData);
        }

        void AddMultipleRenderPass(const MultipleRenderPassData& passesData) {
            order.push_back({ "MultipleRenderPassData", multipleRenderPasses.size() });
            multipleRenderPasses.push_back(passesData);
        }

        void Execute(ES::Engine::Core &core) {
            for (const auto& [type, index] : order) {
                if (type == "RenderPassData") {
                    executePass(singleRenderPasses[index], core);
                } else if (type == "MultipleRenderPassData") {
                    auto &multiplePass = multipleRenderPasses[index];
                    // TODO: find a way to have a better resource management than ugly and unsafe std::function
                    if (multiplePass.preMultiplePassCallback.has_value()) multiplePass.preMultiplePassCallback.value()(core, multiplePass.pass);
                    for (size_t i = 0; i < multiplePass.getNumberOfPass(core); i++) {
                        if (multiplePass.prePassCallback.has_value()) multiplePass.prePassCallback.value()(core, multiplePass.pass);
                        executePass(multiplePass.pass, core);
                        if (multiplePass.postPassCallback.has_value()) multiplePass.postPassCallback.value()(core, multiplePass.pass);
                    }
                    if (multiplePass.postMultiplePassCallback.has_value()) multiplePass.postMultiplePassCallback.value()(core, multiplePass.pass);
                } else {
                    throw std::runtime_error("Unknown render graph node type.");
                }
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

            std::vector<wgpu::CommandBuffer> commandBuffers;

            wgpu::RenderPassDescriptor renderPassDesc(wgpu::Default);
            std::string renderPassDescLabel = fmt::format("CreateRenderPass::{}::RenderPass", renderPassData.name);
            renderPassDesc.label = wgpu::StringView(renderPassDescLabel);

            std::vector<wgpu::RenderPassColorAttachment> colorAttachments;
            colorAttachments.reserve(renderPassData.outputColorTextureName.size());

            for (const auto& colorTextureName : renderPassData.outputColorTextureName) {
                wgpu::RenderPassColorAttachment renderPassColorAttachment(wgpu::Default);
                {
                    renderPassColorAttachment.view = core.GetResource<TextureManager>().Get(entt::hashed_string(colorTextureName.c_str())).textureView;
                    renderPassColorAttachment.loadOp = renderPassData.loadOp;
                    renderPassColorAttachment.storeOp = wgpu::StoreOp::Store;
                }
                if (renderPassData.clearColor.has_value()) {
                    glm::vec4 clearColor = renderPassData.clearColor.value()(core);
                    renderPassColorAttachment.clearValue = wgpu::Color(
                        clearColor.r,
                        clearColor.g,
                        clearColor.b,
                        clearColor.a
                    );
                }
                colorAttachments.push_back(renderPassColorAttachment);
            }

            renderPassDesc.colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size());
            renderPassDesc.colorAttachments = colorAttachments.data();

            if (renderPassData.outputDepthTextureName.has_value()) {
                wgpu::RenderPassDepthStencilAttachment depthStencilAttachment(wgpu::Default);
                {
                    depthStencilAttachment.view = core.GetResource<TextureManager>().Get(entt::hashed_string(renderPassData.outputDepthTextureName.value().c_str())).textureView;
                    depthStencilAttachment.depthClearValue = 1.0f;
                    depthStencilAttachment.depthLoadOp = renderPassData.loadOp;
                    depthStencilAttachment.depthStoreOp = wgpu::StoreOp::Store;
                }

                renderPassDesc.depthStencilAttachment = &depthStencilAttachment;
            }

            wgpu::RenderPassEncoder renderPass = commandEncoder.beginRenderPass(renderPassDesc);

            if (renderPassData.shaderName.has_value()) {
                PipelineData &pipelineData = core.GetResource<Pipelines>().renderPipelines[renderPassData.shaderName.value()];
                renderPass.setPipeline(pipelineData.pipeline);

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
            }
            // Select which render pipeline to use
            if (renderPassData.uniqueRenderCallback.has_value()) { // Find a way to handle this properly, PS: this is used for ImGUI
                renderPassData.uniqueRenderCallback.value()(renderPass, core);
            } else {
                core.GetRegistry().view<ES::Plugin::WebGPU::Component::Mesh, ES::Plugin::Object::Component::Transform>().each([&](auto e, ES::Plugin::WebGPU::Component::Mesh &mesh, ES::Plugin::Object::Component::Transform &transform) {
                    if (!mesh.enabled || mesh.pipelineType != renderPassData.pipelineType) return;

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
            commandBuffers.push_back(commandEncoder.finish(cmdBufferDescriptor));
            commandEncoder.release();

            queue.submit(static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
            for (auto &command : commandBuffers) {
                command.release();
            }
        }

        std::vector<RenderPassData> singleRenderPasses;
        std::vector<MultipleRenderPassData> multipleRenderPasses;
        std::list<std::pair<std::string, size_t>> order;
};
