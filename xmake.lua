add_rules("mode.debug", "mode.release")
add_requires("glm >=1.0.1", "glfw >=3.4", "vcpkg::gl3w >=v2018-05-31#5")

local project_name = "your_project_name"

target(project_name)
    set_kind("binary")
    set_default(true)
    add_packages("glm", "glfw", "vcpkg::gl3w")

    add_files("src/**.cpp")

    set_rundir("$(projectdir)")
