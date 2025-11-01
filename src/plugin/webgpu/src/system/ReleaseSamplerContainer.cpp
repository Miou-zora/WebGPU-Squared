#include "ReleaseSamplerContainer.hpp"
#include "resource/SamplerContainer.hpp"

namespace ES::Plugin::WebGPU::System
{

// Required ?
void ReleaseSamplerContainer(ES::Engine::Core &core) {
    ES::Utils::Log::Debug("Releasing SamplerContainer...");

    auto &samplerContainer = core.GetResource<Resource::SamplerContainer>();

    samplerContainer.Release();
}

} // namespace ES::Plugin::WebGPU::System

