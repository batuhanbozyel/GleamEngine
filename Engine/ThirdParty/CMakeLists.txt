include (${CMAKE_SOURCE_DIR}/CMake/CMakeUtils.cmake)

# Set defaults preventing destination file conflicts
set(SDL_CMAKE_DEBUG_POSTFIX "" CACHE STRING "Name suffix for debug builds" FORCE)
set(SDL_EXTRA_LIBS "" CACHE STRING "SDL library dependencies" FORCE PARENT_SCOPE)

# Disable SDL targets
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)

# Disable rapidjson targets
set(RAPIDJSON_BUILD_DOC OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_BUILD_CXX11 OFF CACHE BOOL "" FORCE)

add_subdirectory(fmt)
add_subdirectory(entt)
add_subdirectory(refl-cpp)
add_subdirectory(rapidjson)
add_subdirectory(googletest)
add_subdirectory(SDL EXCLUDE_FROM_ALL)

set_target_properties (
    SDL3-static
    gmock
    gmock_main
    gtest
    gtest_main
    RapidJSON
    travis_doc
    refl-cpp
    fmt
    EnTT
    PROPERTIES FOLDER Dependencies
)