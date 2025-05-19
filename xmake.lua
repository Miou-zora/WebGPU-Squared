add_rules("mode.debug", "mode.release")
add_requires("wgpu-native ^24.0.0", {configs = {shared = true}})

local project_name = "e2-wgpu"

set_languages("c++20")

target(project_name)
    set_kind("binary")
    set_default(true)

    add_files("src/**.cpp")

    set_rundir("$(projectdir)")
