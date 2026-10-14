include (${CMAKE_SOURCE_DIR}/CMake/CMakeUtils.cmake)

set(DXC_DIRECTORY ${CMAKE_SOURCE_DIR}/Tools/dxc)

add_library(ShaderCompiler STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ShaderCompiler.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ShaderCompiler.cpp
)
target_include_directories(ShaderCompiler PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(ShaderCompiler SYSTEM PRIVATE ${DXC_DIRECTORY}/include)
target_compile_definitions(ShaderCompiler PRIVATE "NOMINMAX")
GLEAM_SET_WARNINGS(ShaderCompiler)

if (APPLE)
    target_link_libraries(ShaderCompiler PUBLIC ${DXC_DIRECTORY}/lib/libdxcompiler.dylib)
else()
    DEPLOY_FILE(${DXC_DIRECTORY}/bin/dxcompiler.dll)
    DEPLOY_FILE(${DXC_DIRECTORY}/bin/dxil.dll)
endif()

add_executable(ShaderCompilerCLI ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
target_link_libraries(ShaderCompilerCLI PRIVATE ShaderCompiler)
GLEAM_SET_WARNINGS(ShaderCompilerCLI)

set_target_properties(
    ShaderCompiler
    ShaderCompilerCLI
    PROPERTIES FOLDER Tools
)
