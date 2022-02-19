include (${CMAKE_SOURCE_DIR}/CMake/CMakeUtils.cmake)

file(GLOB_RECURSE MSL_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.metal)
file(GLOB_RECURSE HLSL_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hlsl)
set(SHADER_FILES ${MSL_FILES} ${HLSL_FILES})

file(GLOB_RECURSE SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.mm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
)

if (WIN32)
    set(INCLUDE_DIRS_RUNTIME
        src
        ${Vulkan_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/SDL2/include
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/fmt/include
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/glm
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/stb
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/imgui
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/ImGuizmo
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/entt/include
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/tinyobjloader
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/OpenFBX/src
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/cgltf
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/volk
    )

    set(LIB_LINKS_RUNTIME
        ${CMAKE_SOURCE_DIR}/bin/$<CONFIG>/fmt.lib
        ${CMAKE_SOURCE_DIR}/bin/$<CONFIG>/volk.lib
        ${CMAKE_SOURCE_DIR}/bin/$<CONFIG>/imgui.lib
    )

    set_source_files_properties(${SHADER_FILES} PROPERTIES VS_TOOL_OVERRIDE "None")

else ()
    set(INCLUDE_DIRS_RUNTIME
        src
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/SDL2/include
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/fmt/include
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/glm
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/stb
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/imgui
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/ImGuizmo
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/entt/include
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/tinyobjloader
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/OpenFBX/src
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/cgltf
    )

    set(LIB_LINKS_RUNTIME
        ${CMAKE_SOURCE_DIR}/bin/$<CONFIG>/libfmt.a
        ${CMAKE_SOURCE_DIR}/bin/$<CONFIG>/libimgui.a
    )
    
endif()

list(APPEND SOURCE_FILES ${SHADER_FILES})
add_library(Runtime STATIC ${SOURCE_FILES})
add_dependencies(Runtime fmt imgui volk)
target_include_directories(Runtime PUBLIC ${INCLUDE_DIRS_RUNTIME})

if (APPLE)
    set_source_files_properties(${SOURCE_FILES} PROPERTIES COMPILE_FLAGS "-x objective-c++")
    set_target_properties(Runtime PROPERTIES MACOSX_BUNDLE YES XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES)
endif()

set_property(TARGET Runtime PROPERTY STATIC_LIBRARY_OPTIONS ${LIB_LINKS_RUNTIME})
target_link_directories(Runtime PUBLIC ${CMAKE_SOURCE_DIR}/bin/$<CONFIG>)
if (WIN32)
    target_link_libraries(Runtime PUBLIC SDL2main.lib SDL2-static.lib)
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

if (WIN32)
    set(HLSL_FILE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src/Renderer/Vulkan/Shaders)
    ADD_GRAPHICS_SHADER(Runtime ${HLSL_FILE_PATH}/FullscreenTriangle.hlsl fullscreenTriangleVertexShader fullscreenTriangleFragmentShader)
    ADD_GRAPHICS_SHADER(Runtime ${HLSL_FILE_PATH}/ForwardPass.hlsl forwardPassVertexShader forwardPassFragmentShader)
endif()