cmake_minimum_required(VERSION 3.13)

include(${CMAKE_SOURCE_DIR}/CMake/GleamConfigurations.cmake)

project(GleamEngine)

set(GLEAM_ENGINE_DIR ${CMAKE_SOURCE_DIR})
include(${CMAKE_SOURCE_DIR}/CMake/GleamSettings.cmake)

    if (WIN32)
        set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Editor)
    endif()

    #preventing in source build
    if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
        message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt.")
    endif()

    #subfolders
    add_subdirectory(Engine/Source/ShaderCompiler)
    add_subdirectory(Engine/Source/Runtime)
    add_subdirectory(Engine/Source/Editor)
    add_subdirectory(Engine/Source/UnitTest)
