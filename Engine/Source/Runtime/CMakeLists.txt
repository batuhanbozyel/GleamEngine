include (${CMAKE_SOURCE_DIR}/CMake/CMakeUtils.cmake)

find_package(Vulkan REQUIRED)
find_package(Python COMPONENTS Interpreter REQUIRED)

# Third-party libraries
add_subdirectory(${CMAKE_SOURCE_DIR}/Engine/ThirdParty ${CMAKE_BINARY_DIR}/ThirdParty)

# Runtime shader files
file(GLOB_RECURSE SHADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hlsl)

# Runtime source files
file(GLOB_RECURSE SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.mm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
)

# Setup link libraries
if (WIN32)
    set(LIB_LINKS SDL3::SDL3-static nlohmann_json::nlohmann_json volk.lib spirv-reflect-static.lib)
    set_source_files_properties(${SHADER_FILES} PROPERTIES VS_TOOL_OVERRIDE "None")
elseif (APPLE)
    find_library(METALIR_LIB libmetalirconverter.dylib)
    set(LIB_LINKS SDL3::SDL3-static nlohmann_json::nlohmann_json ${METALIR_LIB} "-framework UniformTypeIdentifiers")
else()
    set(LIB_LINKS SDL3::SDL3-static nlohmann_json::nlohmann_json volk.a spirv-reflect-static.a)
endif()

# Set Runtime as static library
list(APPEND SOURCE_FILES ${SHADER_FILES})
add_library(Runtime STATIC ${SOURCE_FILES})

# Include directories
if (APPLE)
    add_dependencies(Runtime SDL3-static)
    set(INCLUDE_DIRS
        src
        /usr/local/include
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/SDL/include
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/fmt/include
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/stb
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/entt/single_include
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/tinyobjloader
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/OpenFBX/src
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/cgltf
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/volk
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/SPIRV-Reflect
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/json/single_include
    )
    set_source_files_properties(${SOURCE_FILES} PROPERTIES COMPILE_FLAGS "-x objective-c++")
    set_target_properties(Runtime PROPERTIES MACOSX_BUNDLE YES XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES)
else()
    add_dependencies(Runtime SDL3-static volk spirv-reflect-static)
    set(INCLUDE_DIRS
        src
        ${Vulkan_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/SDL/include
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/fmt/include
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/stb
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/entt/single_include
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/tinyobjloader
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/OpenFBX/src
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/cgltf
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/volk
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/SPIRV-Reflect
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/json/single_include
    )
endif()

# Configure Runtime target
target_include_directories(Runtime PUBLIC ${INCLUDE_DIRS})
target_link_directories(Runtime PUBLIC ${CMAKE_SOURCE_DIR}/bin/$<CONFIG>)
target_link_libraries(Runtime PUBLIC ${LIB_LINKS})
target_precompile_headers(Runtime PUBLIC "src/gpch.h")

# Setup souce grouping and working directory 
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_FILES})
SET_WORKING_DIRECTORY(Runtime ${CMAKE_SOURCE_DIR})

# 
target_compile_definitions(
    Runtime

    PRIVATE
    "_CRT_SECURE_NO_WARNINGS"
    "NOMINMAX"

    PUBLIC
    $<$<CONFIG:Debug>:GDEBUG>
    $<$<CONFIG:Release>:GRELEASE>
)

# Precompile shaders
set(HLSL_FILE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src/Renderer/Shaders)
set(SHADER_FUNCTIONS
    ${HLSL_FILE_PATH}/FullscreenShader.hlsl fullscreenTriangleVertexShader vertex
    ${HLSL_FILE_PATH}/FullscreenShader.hlsl fullscreenTriangleFragmentShader fragment
    ${HLSL_FILE_PATH}/TonemappingShader.hlsl tonemappingFragmentShader fragment
    ${HLSL_FILE_PATH}/ForwardPassShader.hlsl forwardPassVertexShader vertex
    ${HLSL_FILE_PATH}/ForwardPassShader.hlsl forwardPassFragmentShader fragment
    ${HLSL_FILE_PATH}/DebugShader.hlsl debugVertexShader vertex
    ${HLSL_FILE_PATH}/DebugShader.hlsl debugMeshVertexShader vertex
    ${HLSL_FILE_PATH}/DebugShader.hlsl debugFragmentShader fragment
)

add_custom_command(
    TARGET Runtime
    PRE_BUILD
    COMMAND Python::Interpreter ${CMAKE_SOURCE_DIR}/Tools/CompileShaders.py ${CMAKE_SOURCE_DIR}/Assets ${SHADER_FUNCTIONS}
)