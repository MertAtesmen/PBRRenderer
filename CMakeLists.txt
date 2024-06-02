cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_STANDARD 20)


project("OpenGL Project")

add_subdirectory("include")
add_subdirectory("dependencies")

add_executable(main
    src/Main/main.cpp
)

target_link_libraries(main
    "include"
    "dependencies"
)

add_executable(material_picker
    src/MaterialPicker/main.cpp
)

target_link_libraries(material_picker
    "include"
    "dependencies"
)