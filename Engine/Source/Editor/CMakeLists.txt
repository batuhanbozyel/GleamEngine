include (${CMAKE_SOURCE_DIR}/CMake/CMakeUtils.cmake)

set(INCLUDE_DIRS_EDITOR
    src
    ${CMAKE_SOURCE_DIR}/Engine/Source/Runtime/src
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/SDL2/include
)

file(GLOB_RECURSE SOURCE_FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
)

add_executable(Editor WIN32 MACOSX_BUNDLE ${SOURCE_FILES})
add_dependencies(Editor SDL2main SDL2-static Runtime)
target_include_directories(Editor PRIVATE ${INCLUDE_DIRS_EDITOR})

target_link_directories(Editor PRIVATE ${CMAKE_SOURCE_DIR}/bin/$<CONFIG>)
if (WIN32)
    target_link_libraries(Editor PRIVATE Runtime.lib SDL2.lib SDL2main.lib)
else()
    target_link_libraries(Editor PRIVATE 
        libRuntime.a
        SDL2.a
        SDL2main.a
        iconv
        "-framework Carbon"
        "-framework IOKit"
        "-framework ForceFeedback"
        "-framework CoreVideo"
        "-framework AudioToolbox"
        "-framework CoreAudio"
        "-framework Cocoa")
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_FILES})
SET_WORKING_DIRECTORY(Editor ${CMAKE_SOURCE_DIR})