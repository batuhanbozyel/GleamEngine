cmake_minimum_required(VERSION 3.13)

set(CMAKE_CONFIGURATION_TYPES Debug Release RelWithDebInfo CACHE TYPE INTERNAL FORCE)

option(ENABLE_ASAN "Enable Address Sanitizer" OFF)

project(GleamEngine)

    if (WIN32)
        set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Editor)
    elseif (APPLE)
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
        set(CMAKE_XCODE_GENERATE_SCHEME true)
        set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY XCODE_SCHEME_EXECUTABLE Editor)
    endif()

    #preventing in source build
    if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
        message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt.")
    endif()

    #enable c++20
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED True)
    set(CMAKE_CXX_EXTENSIONS OFF)

    #check the compiler
    MESSAGE(STATUS "compiler id ${CMAKE_CXX_COMPILER_ID} ${MSVC}")
    # using Clang
    if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
        set(COMMON_CXX_FLAGS "${COMMON_CXX_FLAGS} -g3 -O0 -Wall -pedantic -Wextra -m64 -mavx2 -mfma -ffast-math")
    # using GCC
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        set(COMMON_CXX_FLAGS "${COMMON_CXX_FLAGS} -Wall -pedantic -Wextra -m64 -mavx2 -mfma -ffast-math")
    # using Intel C++
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
    # using Visual Studio C++
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        set(COMMON_CXX_FLAGS "${COMMON_CXX_FLAGS}  /std:c++20 /W4 /arch:AVX2 /fp:fast /MP /DNOMINMAX")
        add_compile_options(/wd4251 /wd4275 /wd4099 /wd4204)
    endif()

    if (ENABLE_ASAN)
        add_compile_options(-fsanitize=address)
        add_link_options(-fsanitize=address)
    endif()

    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIGURATION>)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIGURATION>)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIGURATION>)

    #subfolders
    add_subdirectory(Engine/Source/Runtime)
    add_subdirectory(Engine/Source/Editor)
    add_subdirectory(Engine/Source/UnitTest)