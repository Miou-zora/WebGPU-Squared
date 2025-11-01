#include "SamplerContainer.hpp"
#include "entity/Entity.hpp"

namespace ES::Plugin::WebGPU::Resource {

wgpu::Sampler SamplerContainer::CreateSampler(const wgpu::AddressMode &addressMode) {
    wgpu::SamplerDescriptor samplerDesc;

    samplerDesc.addressModeU = addressMode;
    samplerDesc.addressModeV = addressMode;
    samplerDesc.addressModeW = addressMode;
    samplerDesc.magFilter = wgpu::FilterMode::Linear;
    samplerDesc.minFilter = wgpu::FilterMode::Linear;
    samplerDesc.mipmapFilter = wgpu::MipmapFilterMode::Linear;
    samplerDesc.lodMinClamp = 0.0f;
    samplerDesc.lodMaxClamp = 1.0f;
    samplerDesc.compare = wgpu::CompareFunction::Undefined;
    samplerDesc.maxAnisotropy = 1;

    return _device.createSampler(samplerDesc);
}

wgpu::Sampler SamplerContainer::CreateSampler(const wgpu::SamplerDescriptor &samplerDesc) {
    return _device.createSampler(samplerDesc);
}

wgpu::Sampler SamplerContainer::CreateNamedSampler(const std::string &name, const wgpu::SamplerDescriptor &samplerDesc) {
    if (_samplers.find(name) != _samplers.end()) {
        ES::Utils::Log::Warn(fmt::format("CreateNamedSampler: Sampler with name {} already exists, entry returned.", name));
        return _samplers[name];
    }
    
    wgpu::Sampler sampler;
    sampler = _device.createSampler(samplerDesc);
    _samplers[name] = sampler;
    return sampler;
}

wgpu::Sampler SamplerContainer::GetSampler(const std::string &name) const {
    auto it = _samplers.find(name);
    if (it != _samplers.end()) {
        return it->second;
    }
    ES::Utils::Log::Warn(fmt::format("GetSampler: Sampler wth name {} not found.", name));
    return nullptr;
}

bool SamplerContainer::HasSampler(const std::string &name) const {
    return _samplers.find(name) != _samplers.end();
}

void SamplerContainer::DestroySampler(const std::string &name) {
    auto it = _samplers.find(name);
    if (it != _samplers.end()) {
        _samplers.erase(it);
    }
    ES::Utils::Log::Error(fmt::format("DestroySampler: Could not destroy sampler with name {}, not found.", name));
}

void SamplerContainer::Release() {
    _samplers.clear();
}

} // namespace ES::Plugin::WebGPU

