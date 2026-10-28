option(ENABLE_ASAN "Enable Address Sanitizer" OFF)

if (MSVC)
    set(CMAKE_C_FLAGS_DEVELOPMENT "/MD /Zi /Ob0 /Od /RTC1")
    set(CMAKE_CXX_FLAGS_DEVELOPMENT "/MD /Zi /Ob0 /Od /RTC1")
    set(CMAKE_EXE_LINKER_FLAGS_DEVELOPMENT "/debug /INCREMENTAL")
    set(CMAKE_SHARED_LINKER_FLAGS_DEVELOPMENT "/debug /INCREMENTAL")
else()
    set(CMAKE_C_FLAGS_DEVELOPMENT "-O0 -g")
    set(CMAKE_CXX_FLAGS_DEVELOPMENT "-O0 -g")
endif()

if (APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
endif()

#enable c++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

#check the compiler
MESSAGE(STATUS "compiler id ${CMAKE_CXX_COMPILER_ID} ${MSVC}")
if (MSVC)
    string(REPLACE "/W3" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    string(REPLACE "/W3" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    add_compile_options(/MP /arch:AVX2 /wd4100 /wd4201 /wd4251 /wd4275 /wd4099 /wd4204)
elseif (NOT APPLE)
    add_compile_options(-mavx2 -mfma)
endif()

add_compile_definitions(EA_DEBUG=0 EASTL_DEBUG=0 EASTL_NAME_ENABLED=0)

if (ENABLE_ASAN)
    add_compile_options(-fsanitize=address)
    add_link_options(-fsanitize=address)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIGURATION>)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIGURATION>)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIGURATION>)
