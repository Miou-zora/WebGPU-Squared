add_rules("mode.debug", "mode.release")
add_requires("spdlog", "entt", "fmt", "glm")
add_requires("wgpu-native ^24.0.0", {configs = {shared = false}})
add_requires("glfw ^3.4", { configs = {shared = false} })
add_requires("glfw3webgpu v1.3.0-alpha", {configs = {shared = false}, debug = true})
add_requires("imgui v1.92.0-docking", {configs = {shared = false, glfw = true, wgpu = true, wgpu_backend = "wgpu"}, debug = true})
add_requires("stb")

includes("../../EngineSquared/xmake.lua")

local project_name = "e2-wgpu"

set_languages("c++20")

target(project_name)
    set_kind("binary")
    set_default(true)
    add_packages("wgpu-native")
    add_packages("spdlog", "entt", "fmt", "glm")
    add_packages("imgui")
    add_packages("stb")

    add_deps("EngineSquared")
    add_packages("glfw3webgpu")
    add_defines("IMGUI_IMPL_WEBGPU_BACKEND_WGPU")

    if is_mode("debug") then
        add_defines("DEBUG")
        add_defines("ES_DEBUG")
    end

    add_files("src/**.cpp")
    add_headerfiles("src/**.hpp", { public = true })
    add_includedirs("src/", {public = true})
    add_includedirs("src/plugin", {public = true})
    add_includedirs("src/plugin/system", {public = true})
    add_includedirs("src/plugin/util", {public = true})
    add_includedirs("src/plugin/component", {public = true})

    set_rundir("$(projectdir)")

    -- Enable as many warnings as possible
    -- add_cxxflags("-Wall", "-Wextra", "-Wpedantic", "-Wshadow", "-Wnon-virtual-dtor", "-Wold-style-cast", "-Wcast-align", "-Wunused", "-Woverloaded-virtual", "-Wconversion", "-Wsign-conversion", "-Wmisleading-indentation", "-Wduplicated-cond", "-Wduplicated-branches", "-Wlogical-op", "-Wnull-dereference", "-Wuseless-cast", "-Wdouble-promotion", "-Wformat=2", "-Wno-unused-parameter", {force = true})
    -- if is_plat("macosx") then
    --     add_cxxflags("-Weverything", "-Wno-c++98-compat", "-Wno-c++98-compat-pedantic", {force = true})
    -- end
