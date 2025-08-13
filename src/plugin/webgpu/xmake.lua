add_requires("spdlog", "entt", "fmt", "glm")
add_requires("wgpu-native ^24.0.0", {configs = {shared = false}})
add_requires("glfw ^3.4", { configs = {shared = false} })
add_requires("glfw3webgpu v1.3.0-alpha", {configs = {shared = false}, debug = true})
add_requires("imgui v1.92.0-docking", {configs = {shared = false, glfw = true, wgpu = true, wgpu_backend = "wgpu"}, debug = true})
add_requires("stb")

includes("../../../../../EngineSquared/xmake.lua")

target("PluginWebGPU")
    set_group(PLUGINS_GROUP_NAME)
    set_kind("static")
    set_languages("cxx20")
    add_packages("entt", "glm", "spdlog", "fmt", "glfw", "glew", "stb", "wgpu-native", "glfw3webgpu")
    set_policy("build.warning", true)

    if is_mode("debug") then
        add_defines("DEBUG")
    end

    add_deps("EngineSquared")

    add_files("src/**.cpp")

    add_includedirs("src/", {public = true})
    add_includedirs("src/util/", {public = true})
    add_includedirs("src/component/", {public = true})
    add_includedirs("src/system/", {public = true})
    add_includedirs("src/plugin/", {public = true})
    add_includedirs("src/resource/", {public = true})
    add_headerfiles("src/**.hpp")
