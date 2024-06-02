add_library(stb_image STATIC 
    stb_image/stb_image.cpp
)

add_library(glad STATIC
    glad/glad.c
)

add_library(imgui STATIC
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_demo.cpp
    imgui/backends/imgui_impl_glfw.cpp
    imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PRIVATE "imgui" "imgui/backends")


target_include_directories(glad PRIVATE "../include")

add_subdirectory(glfw-3.4)
add_subdirectory(assimp)

add_library("dependencies" INTERFACE)

target_link_libraries("dependencies" INTERFACE
    stb_image
    assimp
    glad
    glfw
    imgui
)