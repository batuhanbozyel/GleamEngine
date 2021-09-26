include (${CMAKE_SOURCE_DIR}/CMake/CMakeUtils.cmake)

set(INCLUDE_DIRS_RUNTIME
    src
    ${Vulkan_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/SDL2/include
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/spdlog/include
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/glm
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/stb
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/imgui
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/ImGuizmo
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/entt/include
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/tinyobjloader
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/OpenFBX/src
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/tinygltf
)

if (WIN32)
    set(LIB_LINKS_RUNTIME
        ${Vulkan_LIBRARIES}
        ${CMAKE_SOURCE_DIR}/bin/$<CONFIG>/imgui.lib
    )
else ()
    set(LIB_LINKS_RUNTIME
        $ENV{VULKAN_SDK}/../MoltenVK/MoltenVK.xcframework/macos-arm64_x86_64/libMoltenVK.a
        ${CMAKE_SOURCE_DIR}/bin/$<CONFIG>/libimgui.a
    )
endif()

file(GLOB_RECURSE SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
)

add_library(Runtime STATIC ${SOURCE_FILES})
add_dependencies(Runtime imgui)
target_include_directories(Runtime PUBLIC ${INCLUDE_DIRS_RUNTIME})

set_property(TARGET Runtime PROPERTY STATIC_LIBRARY_OPTIONS ${LIB_LINKS_RUNTIME})
target_link_directories(Runtime PUBLIC ${CMAKE_SOURCE_DIR}/bin/$<CONFIG>)
if (WIN32)
    target_link_libraries(Runtime PUBLIC SDL2main.lib SDL2.lib)
else()
    target_link_libraries(Runtime PUBLIC SDL2main.a SDL2.a)
endif()

target_precompile_headers(Runtime PUBLIC "src/gpch.h")

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_FILES})
SET_WORKING_DIRECTORY(Runtime ${CMAKE_SOURCE_DIR})

target_compile_definitions(
    Runtime

    PRIVATE
    "_CRT_SECURE_NO_WARNINGS"

    PUBLIC
    "GLM_FORCE_DEPTH_ZERO_TO_ONE"
    $<$<CONFIG:Debug>:GDEBUG>
    $<$<CONFIG:RelWithDebugInfo>:GDEBUG>
    $<$<CONFIG:Release>:GRELEASE>
)