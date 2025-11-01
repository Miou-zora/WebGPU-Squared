#pragma once

#include <unordered_map>
#include <string>
#include "webgpu.hpp"

namespace ES::Plugin::WebGPU::Resource {

class SamplerContainer {
public:
    explicit SamplerContainer(wgpu::Device device) : _device(device) {}
    ~SamplerContainer() = default;

    wgpu::Sampler CreateSampler(const wgpu::AddressMode &addressMode = wgpu::AddressMode::ClampToEdge);
    wgpu::Sampler CreateSampler(const wgpu::SamplerDescriptor &samplerDesc);
    wgpu::Sampler CreateNamedSampler(const std::string &name, const wgpu::SamplerDescriptor &samplerDesc);

    wgpu::Sampler GetSampler(const std::string &name) const;
    wgpu::Device GetDevice() const { return _device; }
    
    bool HasSampler(const std::string &name) const;

    void DestroySampler(const std::string &name);
    void Release();

private:
    wgpu::Device _device;
    std::unordered_map<std::string, wgpu::Sampler> _samplers;
};

} // namespace ES::Plugin::WebGPU