include (${CMAKE_SOURCE_DIR}/CMake/CMakeUtils.cmake)

find_package(Python COMPONENTS Interpreter REQUIRED)

# Add MikkTSpace as static library
add_library(MikkTSpace STATIC ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/MikkTSpace/mikktspace.c)
set_target_properties(MikkTSpace PROPERTIES FOLDER Dependencies)
target_include_directories(MikkTSpace PRIVATE ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/MikkTSpace)

set(INCLUDE_DIRS
    src
    ${CMAKE_SOURCE_DIR}/Engine/Source/Runtime/src
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/EASTL/include
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/GleamReflect/include
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/SDL/include
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/fmt/include
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/rapidjson/include
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/entt/src
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/cgltf
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/MikkTSpace
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/v-hacd/include
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/stb
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/meshoptimizer/src
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
else()
    set_source_files_properties(${SOURCE_FILES} ${HEADER_FILES} PROPERTIES COMPILE_FLAGS "-x objective-c++")
endif()
list(APPEND SOURCE_FILES ${SHADER_FILES})

# Editor reflection module
file(GLOB_RECURSE HEADER_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp
)
list(FILTER HEADER_FILES EXCLUDE REGEX ".*Reflection\\.generated\\.h$")

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
    target_link_libraries(Editor PRIVATE Runtime.lib MikkTSpace.lib meshoptimizer ${GLEAM_RUNTIME_LIBS})
else()
    target_link_libraries(Editor PRIVATE Runtime.a MikkTSpace.a meshoptimizer ${GLEAM_RUNTIME_LIBS})
endif()

add_dependencies(Editor Runtime MikkTSpace meshoptimizer)

# CppReflection
set(EDITOR_REFLECTION_COMPILER_ARGS ${GLEAM_REFLECTION_COMPILER_ARGS} "--extra-arg=-I${CMAKE_CURRENT_SOURCE_DIR}/src")
foreach(INCLUDE_DIR ${INCLUDE_DIRS})
    if (IS_ABSOLUTE ${INCLUDE_DIR})
        list(APPEND EDITOR_REFLECTION_COMPILER_ARGS "--extra-arg=-I${INCLUDE_DIR}")
    endif()
endforeach()

GLEAM_ADD_REFLECTION_MODULE(
    NAME Editor
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src
    BINARY_DIR ${GLEAM_REFLECTION_BINARY_DIR}
    HEADERS ${HEADER_FILES}
    DEPENDS Runtime
    COMPILER_ARGS ${EDITOR_REFLECTION_COMPILER_ARGS}
)
add_dependencies(EditorReflection RuntimeReflection)
add_dependencies(Editor EditorReflection)

# The application loads every module's types from one linked database
GLEAM_LINK_REFLECTION_MODULES(
    OUTPUT ${CMAKE_SOURCE_DIR}/Gleam.Reflection.db
    BINARY_DIR ${GLEAM_REFLECTION_BINARY_DIR}
    MODULES Runtime Editor
)
add_dependencies(Editor ReflectionLink)

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
set(SHADER_STAMP_FILE "${CMAKE_BINARY_DIR}/editor_shader_compile.stamp")
add_custom_command(
    OUTPUT ${SHADER_STAMP_FILE}
    COMMAND Python::Interpreter ${CMAKE_SOURCE_DIR}/Tools/CompileShaders.py
        "-d" ${HLSL_FILE_PATH} 
        $<$<CONFIG:Debug>:--debug>
    COMMAND ${CMAKE_COMMAND} -E touch ${SHADER_STAMP_FILE}
    DEPENDS ${HEADER_FILES} ${SHADER_FILES}
    COMMENT "Compiling editor shaders..."
    VERBATIM
    COMMAND_EXPAND_LISTS
)
add_custom_target(EditorShaders DEPENDS ${SHADER_STAMP_FILE})
add_dependencies(Editor EditorShaders)

set_target_properties(
    EditorShaders
    EditorReflection
    PROPERTIES FOLDER CMakeCustomRules
)