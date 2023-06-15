include (${CMAKE_SOURCE_DIR}/CMake/CMakeUtils.cmake)

set(BUILD_SHARED_LIBS OFF)

add_subdirectory(fmt)
add_subdirectory(entt)
add_subdirectory(SDL)
add_subdirectory(volk)
add_subdirectory(googletest)
add_subdirectory(SPIRV-Reflect)

set_target_properties (
    SDL3-static
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