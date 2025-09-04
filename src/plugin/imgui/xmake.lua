add_requires("wgpu-native ^24.0.0", {configs = {shared = false}})
add_requires("glfw ^3.4", { configs = {shared = false} })
add_requires("glfw3webgpu v1.3.0-alpha", {configs = {shared = false}, debug = true})
add_requires("imgui v1.92.0-docking", {configs = {shared = false, glfw = true, wgpu = true, wgpu_backend = "wgpu"}, debug = true})
add_requires("stb")

add_repositories("package_repo https://github.com/EngineSquared/xrepo.git")

add_requires("enginesquared webgpu")

includes("../webgpu/xmake.lua")

target("PluginImGUI")
    set_group(PLUGINS_GROUP_NAME)
    set_kind("static")
    set_languages("cxx20")
    set_policy("build.warning", true)

    add_deps("PluginWebGPU")
    add_packages("glfw", "glew", "stb", "wgpu-native", "glfw3webgpu", "lodepng")
    add_packages("enginesquared")
    add_packages("imgui")
    add_defines("IMGUI_IMPL_WEBGPU_BACKEND_WGPU")


    add_files("src/**.cpp")

    add_includedirs("src/", {public = true})
    add_includedirs("src/util/", {public = true})
    add_includedirs("src/plugin/", {public = true})
    add_headerfiles("src/**.hpp")

