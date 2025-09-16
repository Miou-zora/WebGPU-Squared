#include "RmluiWebgpu.hpp"
#include "RenderingPipeline.hpp"

namespace ES::Plugin::Rmlui {
    void Plugin::Bind() {
        RequirePlugins<ES::Plugin::RenderingPipeline::Plugin>();

        RegisterResource(Resource::UIContext());
    }
}
