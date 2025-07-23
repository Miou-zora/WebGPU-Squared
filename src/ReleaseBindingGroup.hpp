#pragma once

#include "webgpu.hpp"
#include "Engine.hpp"

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
