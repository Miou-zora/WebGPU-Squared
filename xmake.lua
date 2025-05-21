add_repositories("my-repo glfw")

add_rules("mode.debug", "mode.release")
add_requires("wgpu-native ^24.0.0", {configs = {shared = true}})
add_requires("glfw ^3.4", { configs = {shared = true} })
add_requires("glfw3webgpu v1.3.0-alpha", {configs = {shared = true} })
add_requires("spdlog", "entt", "fmt", "glm")

includes("../../EngineSquared/xmake.lua")

local project_name = "e2-wgpu"

set_languages("c++20")

target("webgpuglfw")
    set_kind("$(kind)")
    set_languages("c11")
    add_headerfiles("src/glfw3webgpu.h")

    add_mxflags("-fno-objc-arc")

    add_packages("wgpu-native")
    add_packages("glfw")

    if is_plat("macosx") then
        add_frameworks("Metal", "Foundation", "QuartzCore", "Cocoa")
        add_defines("_GLFW_COCOA")
    end

    add_files("src/glfw3webgpu.m")

target(project_name)
    set_kind("binary")
    set_default(true)
    add_packages("wgpu-native", "glfw")
    add_packages("spdlog", "entt", "fmt", "glm")

    add_deps("EngineSquared")
    add_deps("webgpuglfw")
    -- add_packages("glfw3webgpu")

    add_files("src/**.cpp")

    set_rundir("$(projectdir)")

    add_defines("_GLFW_COCOA")
