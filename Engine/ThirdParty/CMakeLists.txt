include (${CMAKE_SOURCE_DIR}/CMake/CMakeUtils.cmake)

# Set defaults preventing destination file conflicts
set(SDL_CMAKE_DEBUG_POSTFIX "" CACHE STRING "Name suffix for debug builds" FORCE)
set(SDL_EXTRA_LIBS "" CACHE STRING "SDL library dependencies" FORCE PARENT_SCOPE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)

set(SPIRV_REFLECT_STATIC_LIB ON CACHE BOOL "" FORCE)
set(SPIRV_REFLECT_EXECUTABLE OFF CACHE BOOL "" FORCE)
set(SPIRV_REFLECT_EXAMPLES OFF CACHE BOOL "" FORCE)

set(JSON_BuildTests OFF CACHE INTERNAL "")

add_subdirectory(fmt)
add_subdirectory(entt)
add_subdirectory(volk)
add_subdirectory(json)
add_subdirectory(googletest)
add_subdirectory(SPIRV-Reflect)
add_subdirectory(SDL EXCLUDE_FROM_ALL)
add_subdirectory(VulkanMemoryAllocator)

set_target_properties (
    SDL3-static
    gmock
    gmock_main
    gtest
    gtest_main
    nlohmann_json
    volk
    fmt
    EnTT
    spirv-reflect-static
    VulkanMemoryAllocator
    PROPERTIES FOLDER Dependencies
)