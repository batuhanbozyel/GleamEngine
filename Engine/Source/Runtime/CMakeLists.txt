include (${CMAKE_SOURCE_DIR}/CMake/CMakeUtils.cmake)

find_package(Python COMPONENTS Interpreter REQUIRED)

# Third-party libraries
add_subdirectory(${CMAKE_SOURCE_DIR}/Engine/ThirdParty ${CMAKE_BINARY_DIR}/ThirdParty)

# Runtime files
file(GLOB_RECURSE SHADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hlsl*)
file(GLOB_RECURSE SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.mm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.m
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp
)
list(FILTER SOURCE_FILES EXCLUDE REGEX ".*Reflection\\.generated\\.h$")

# Add imgui as static library
add_library(imgui STATIC
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/imgui/imgui.cpp
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/imgui/imgui_draw.cpp
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/imgui/imgui_widgets.cpp
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/imgui/imgui_demo.cpp
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/imgui/imgui_tables.cpp
)

# Configure imgui
target_include_directories(imgui PRIVATE
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/imgui
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/SDL/include
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/DirectXAgilitySDK/build/native/include
)
set_target_properties (imgui PROPERTIES FOLDER Dependencies)

# Setup link libraries
if (WIN32)
    set(LIB_LINKS SDL3::SDL3-static d3d12.lib dxgi.lib dxguid.lib dstorage.lib imgui.lib Ole32.lib)
    set_source_files_properties(${SHADER_FILES} PROPERTIES VS_TOOL_OVERRIDE "None")
elseif (APPLE)
    find_library(METALIR_LIB libmetalirconverter.dylib)
    set(LIB_LINKS SDL3::SDL3-static imgui ${METALIR_LIB} "-framework UniformTypeIdentifiers")
endif()

# Set Runtime as static library
add_library(Runtime STATIC ${SOURCE_FILES} ${SHADER_FILES})
add_dependencies(Runtime SDL3-static imgui)

# Include directories
if (APPLE)
    set(INCLUDE_DIRS
        /usr/local/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/SDL/include
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/fmt/include
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/stb
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/entt/src
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/rapidjson/include
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/GleamReflect/include
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/imgui
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/ImGuizmo
    )
    set_source_files_properties(${SOURCE_FILES} PROPERTIES COMPILE_FLAGS "-x objective-c++")
    set_target_properties(Runtime PROPERTIES MACOSX_BUNDLE YES XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES)
else()
    set(INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/SDL/include
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/fmt/include
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/stb
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/entt/src
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/rapidjson/include
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/GleamReflect/include
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/DirectStorage/native/include
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/DirectXAgilitySDK/build/native/include
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/imgui
        ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/ImGuizmo
    )
endif()

# Configure Runtime target
target_include_directories(Runtime PUBLIC ${INCLUDE_DIRS})
target_link_directories(Runtime PUBLIC ${CMAKE_BINARY_DIR}/$<CONFIG>)
target_link_libraries(Runtime PUBLIC ${LIB_LINKS})
target_precompile_headers(Runtime PUBLIC "src/gpch.h")

# Setup souce grouping and working directory 
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_FILES} ${SHADER_FILES})
SET_WORKING_DIRECTORY(Runtime ${CMAKE_SOURCE_DIR})

# 
target_compile_definitions(
    Runtime

    PRIVATE
    "_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR"
    "_CRT_SECURE_NO_WARNINGS"
    "WIN32_LEAN_AND_MEAN"
    "NOMINMAX"

    PUBLIC
    $<$<CONFIG:Debug>:GDEBUG>
    $<$<CONFIG:Release>:GRELEASE>
)

DEPLOY_FILE(${CMAKE_SOURCE_DIR}/Engine/ThirdParty/DirectStorage/native/lib/x64/dstorage.lib)
DEPLOY_FILE(${CMAKE_SOURCE_DIR}/Engine/ThirdParty/DirectStorage/native/bin/x64/dstorage.dll)
DEPLOY_FILE(${CMAKE_SOURCE_DIR}/Engine/ThirdParty/DirectStorage/native/bin/x64/dstoragecore.dll)

DEPLOY_FILE_TO(${CMAKE_SOURCE_DIR}/Engine/ThirdParty/DirectXAgilitySDK/build/native/bin/x64/D3D12Core.dll D3D12)
DEPLOY_FILE_TO(${CMAKE_SOURCE_DIR}/Engine/ThirdParty/DirectXAgilitySDK/build/native/bin/x64/d3d12SDKLayers.dll D3D12)

# CppReflection
set(REFLECTION_COMPILER_ARGS "--extra-arg=-working-directory=${CMAKE_SOURCE_DIR}")
if(APPLE)
    list(APPEND REFLECTION_COMPILER_ARGS "--extra-arg-before=-ObjC++")
else()
    list(APPEND REFLECTION_COMPILER_ARGS "--extra-arg-before=--language=c++")
endif()
list(APPEND REFLECTION_COMPILER_ARGS "--extra-arg=-std=c++20")
list(APPEND REFLECTINO_COMPILER_ARGS "--extra-arg=--no-warnings")
list(APPEND REFLECTION_COMPILER_ARGS "--extra-arg=-D_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR")
list(APPEND REFLECTION_COMPILER_ARGS "--extra-arg=-D_CRT_SECURE_NO_WARNINGS")
list(APPEND REFLECTION_COMPILER_ARGS "--extra-arg=-DWIN32_LEAN_AND_MEAN")
list(APPEND REFLECTION_COMPILER_ARGS "--extra-arg=-DNOMINMAX")

foreach(INCLUDE_DIR ${INCLUDE_DIRS})
    list(APPEND REFLECTION_COMPILER_ARGS "--extra-arg=-I${INCLUDE_DIR}")
endforeach()

string(REPLACE ";" " " REFLECTION_FILES ${SOURCE_FILES})
string(REPLACE ";" " " REFLECTION_ARGS ${REFLECTION_COMPILER_ARGS})

add_custom_command(
    TARGET Runtime
    PRE_BUILD
    COMMAND ${CMAKE_SOURCE_DIR}/Tools/CppReflection
        "--module=Runtime"
        "--header-dir=${CMAKE_CURRENT_SOURCE_DIR}/src"
        "--binary-dir=${CMAKE_SOURCE_DIR}"
        ${REFLECTION_COMPILER_ARGS}
        ${SOURCE_FILES}
    COMMAND_EXPAND_LISTS
)

# Precompile shaders
set(HLSL_FILE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src/Renderer/Shaders)
add_custom_command(
    TARGET Runtime
    PRE_BUILD
    COMMAND Python::Interpreter ${CMAKE_SOURCE_DIR}/Tools/CompileShaders.py
        "-d" ${HLSL_FILE_PATH} 
        $<$<CONFIG:Debug>:--debug>
)