include (${CMAKE_SOURCE_DIR}/CMake/CMakeUtils.cmake)

add_subdirectory(fmt)
add_subdirectory(entt)
add_subdirectory(SDL2)
add_subdirectory(volk)
add_subdirectory(googletest)
add_subdirectory(SPIRV-Reflect)

set_target_properties (
    SDL2
    SDL2main
    SDL2-static
    gmock
    gmock_main
    gtest
    gtest_main
    volk
    fmt
    EnTT
    spirv-reflect-static
    PROPERTIES FOLDER Dependencies
)