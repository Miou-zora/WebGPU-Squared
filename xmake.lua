add_repositories("my-repo glfw")

add_rules("mode.debug", "mode.release")
add_requires("wgpu-native ^24.0.0", {configs = {shared = true}})
add_requires("glfw ^3.4", {configs = {shared = true}})
-- We precise commit hash as the release is not up to date
add_requires("glfw3webgpu v1.3.0-alpha", {configs = {shared = true}})
add_requires("spdlog", "entt", "fmt", "glm")

includes("../../EngineSquared/xmake.lua")

local project_name = "e2-wgpu"

set_languages("c++20")

target(project_name)
    set_kind("binary")
    set_default(true)
    add_packages("wgpu-native", "glfw", "glfw3webgpu")
    add_packages("spdlog", "entt", "fmt", "glm")

    add_deps("EngineSquared")

    add_files("src/**.cpp")

    set_rundir("$(projectdir)")
