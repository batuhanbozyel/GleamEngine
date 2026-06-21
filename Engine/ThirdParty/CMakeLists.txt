include (${CMAKE_SOURCE_DIR}/CMake/CMakeUtils.cmake)
include (${CMAKE_SOURCE_DIR}/CMake/PreseedClangCache.cmake)

# Set defaults preventing destination file conflicts
set(SDL_CMAKE_DEBUG_POSTFIX "" CACHE STRING "Name suffix for debug builds" FORCE)
set(SDL_EXTRA_LIBS "" CACHE STRING "SDL library dependencies" FORCE PARENT_SCOPE)

# Disable SDL targets
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)

# Disable rapidjson targets
set(RAPIDJSON_BUILD_DOC OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_BUILD_CXX11 OFF CACHE BOOL "" FORCE)

# Enable entt natvis
set(ENTT_INCLUDE_NATVIS ON CACHE BOOL "" FORCE)

# Disable meshoptimizer targets
set(MESHOPT_BUILD_DEMO OFF CACHE BOOL "" FORCE)
set(MESHOPT_BUILD_GLTFPACK OFF CACHE BOOL "" FORCE)
set(MESHOPT_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(MESHOPT_INSTALL OFF CACHE BOOL "" FORCE)

# Add imgui as static library
add_library(imgui STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_draw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_widgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_demo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_tables.cpp
)
target_include_directories(imgui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/SDL/include
    ${CMAKE_CURRENT_SOURCE_DIR}/DirectXAgilitySDK/build/native/include
)

add_subdirectory(fmt)
add_subdirectory(entt)
add_subdirectory(EASTL)
add_subdirectory(rapidjson)
add_subdirectory(googletest)
add_subdirectory(GleamReflect)
add_subdirectory(SDL EXCLUDE_FROM_ALL)
add_subdirectory(meshoptimizer)

set_target_properties (
    SDL3-static
    SDL_uclibc
    EASTL
    imgui
    gmock
    gmock_main
    gtest
    gtest_main
    RapidJSON
    travis_doc
    fmt
    EnTT
    GleamReflect
    meshoptimizer
    PROPERTIES FOLDER Dependencies
)