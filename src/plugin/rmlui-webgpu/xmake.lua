includes("../webgpu/xmake.lua")

target("PluginRmluiWebgpu")
    set_group(PLUGINS_GROUP_NAME)
    set_kind("static")
    set_languages("cxx20")
    set_policy("build.warning", true)

    add_deps("PluginWebGPU")
    add_packages("glfw", "glew", "stb", "wgpu-native", "glfw3webgpu", "lodepng")
    add_packages("enginesquared")

    add_files("src/**.cpp")

    add_includedirs("src/", {public = true})

    add_headerfiles("src/(**.hpp)")
    add_headerfiles("src/(plugin/**.hpp)")
    add_headerfiles("src/(system/**.hpp)")
    add_headerfiles("src/(resource/**.hpp)")
    add_headerfiles("src/(utils/**.hpp)")
