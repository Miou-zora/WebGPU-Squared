#include "ReleaseBindingGroup.hpp"
#include "structs.hpp"

namespace ES::Plugin::WebGPU::System {

void ReleaseBindingGroup(ES::Engine::Core &core)
{
	auto &bindGroups = core.GetResource<BindGroups>();

	for (auto &[key, bindGroup] : bindGroups.groups) {
		if (bindGroup) {
			bindGroup.release();
			bindGroup = nullptr;
		}
	}
}
}
