include (${CMAKE_SOURCE_DIR}/CMake/CMakeUtils.cmake)

add_subdirectory(fmt)
add_subdirectory(entt)
add_subdirectory(SDL2)
add_subdirectory(volk)
add_subdirectory(googletest)
add_subdirectory(SPIRV-Reflect)

if (WIN32)
    add_library(imgui STATIC
        "imgui/imgui.cpp"
        "imgui/imgui_draw.cpp"
        "imgui/imgui_widgets.cpp"
        "imgui/imgui_demo.cpp"
        "imgui/imgui_tables.cpp"
        "imgui/backends/imgui_impl_sdl2.cpp"
        "imgui/backends/imgui_impl_vulkan.cpp"
    )
else()
    add_library(imgui STATIC
        "imgui/imgui.cpp"
        "imgui/imgui_draw.cpp"
        "imgui/imgui_widgets.cpp"
        "imgui/imgui_demo.cpp"
        "imgui/imgui_tables.cpp"
        "imgui/backends/imgui_impl_sdl2.cpp"
        "imgui/backends/imgui_impl_metal.mm"
    )
endif()

target_include_directories(imgui PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/imgui
    ${CMAKE_CURRENT_LIST_DIR}/SDL2/include
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