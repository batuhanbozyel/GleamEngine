include (${CMAKE_SOURCE_DIR}/CMake/CMakeUtils.cmake)

find_package(Python COMPONENTS Interpreter REQUIRED)

set(INCLUDE_DIRS
    src
    ${CMAKE_SOURCE_DIR}/Engine/Source/Runtime/src
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/SDL/include
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/fmt/include
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/refl-cpp/include
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/rapidjson/include
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/entt/src
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/cgltf
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/imgui
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/ImGuizmo
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/DirectXAgilitySDK/build/native/include
)

file(GLOB_RECURSE SOURCE_FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c*
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.m*
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h*
)

# Editor shader files
file(GLOB_RECURSE SHADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hlsl*)
if (WIN32)
    set_source_files_properties(${SHADER_FILES} PROPERTIES VS_TOOL_OVERRIDE "None")
endif()
list(APPEND SOURCE_FILES ${SHADER_FILES})

add_executable(Editor WIN32 MACOSX_BUNDLE ${SOURCE_FILES})
target_include_directories(Editor PRIVATE ${INCLUDE_DIRS})
target_link_directories(Editor PRIVATE ${CMAKE_BINARY_DIR}/$<CONFIG>)

if (APPLE)
    set_target_properties(Editor PROPERTIES MACOSX_BUNDLE YES XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES)
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_FILES})
SET_WORKING_DIRECTORY(Editor ${CMAKE_SOURCE_DIR})

# Link Runtime
get_target_property(GLEAM_RUNTIME_LIBS Runtime LINK_LIBRARIES)
if (WIN32)
    target_link_libraries(Editor PRIVATE Runtime.lib ${GLEAM_RUNTIME_LIBS})
else()
    target_link_libraries(Editor PRIVATE Runtime.a ${GLEAM_RUNTIME_LIBS})
endif()

add_dependencies(Editor Runtime)

target_compile_definitions(
    Editor

    PRIVATE
    PYTHON_INTERPRETER=\"${Python_EXECUTABLE}\"
    "_CRT_SECURE_NO_WARNINGS"
    "WIN32_LEAN_AND_MEAN"
    "NOMINMAX"
    
    PUBLIC
    $<$<CONFIG:Debug>:GDEBUG>
    $<$<CONFIG:Release>:GRELEASE>
)

# Precompile shaders
set(HLSL_FILE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src/Renderers/Shaders)
add_custom_command(
    TARGET Editor
    PRE_BUILD
    COMMAND Python::Interpreter ${CMAKE_SOURCE_DIR}/Tools/CompileShaders.py "-d" ${HLSL_FILE_PATH}
)
