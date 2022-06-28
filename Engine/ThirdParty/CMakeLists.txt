include (${CMAKE_SOURCE_DIR}/CMake/CMakeUtils.cmake)

add_subdirectory(fmt)
add_subdirectory(entt)
add_subdirectory(SDL2)
add_subdirectory(volk)
add_subdirectory(googletest)
add_subdirectory(SPIRV-Reflect)

add_library(imgui STATIC
    "imgui/imconfig.h"
    "imgui/imgui.h"
    "imgui/imgui.cpp"
    "imgui/imgui_draw.cpp"
    "imgui/imgui_internal.h"
    "imgui/imgui_widgets.cpp"
    "imgui/imstb_rectpack.h"
    "imgui/imstb_textedit.h"
    "imgui/imstb_truetype.h"
    "imgui/imgui_demo.cpp"
    "imgui/imgui_tables.cpp"
)

set_target_properties (
    SDL2
    SDL2main
    SDL2-static
    gmock
    gmock_main
    gtest
    gtest_main
    imgui
    volk
    fmt
    EnTT
    spirv-reflect-static
    PROPERTIES FOLDER Dependencies
)