include (${CMAKE_SOURCE_DIR}/CMake/CMakeUtils.cmake)

# Set defaults preventing destination file conflicts
set(SDL_CMAKE_DEBUG_POSTFIX "" CACHE STRING "Name suffix for debug builds" FORCE)
set(SDL_EXTRA_LIBS "" CACHE STRING "SDL library dependencies" FORCE PARENT_SCOPE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)

set(JSON_BuildTests OFF CACHE INTERNAL "")
set(D3D12MA_AGILITY_SDK_DIRECTORY ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/DirectXAgilitySDK CACHE STRING "Path to unpacked DX12 Agility SDK." FORCE)

add_subdirectory(fmt)
add_subdirectory(entt)
add_subdirectory(json)
add_subdirectory(googletest)
add_subdirectory(SDL EXCLUDE_FROM_ALL)
add_subdirectory(D3D12MemoryAllocator)

set_target_properties (
    SDL3-static
    gmock
    gmock_main
    gtest
    gtest_main
    nlohmann_json
    fmt
    EnTT
    D3D12MemoryAllocator
    PROPERTIES FOLDER Dependencies
)