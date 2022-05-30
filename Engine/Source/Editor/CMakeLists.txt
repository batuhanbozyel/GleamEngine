include (${CMAKE_SOURCE_DIR}/CMake/CMakeUtils.cmake)

set(INCLUDE_DIRS_EDITOR
    src
    ${CMAKE_SOURCE_DIR}/Engine/Source/Runtime/src
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/SDL2/include
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/fmt/include
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
    target_link_libraries(Editor PRIVATE Runtime.lib SDL2main.lib SDL2-static.lib
        winmm
        version
        Imm32
        Setupapi)
else()
    add_framework(Editor Metal)
    add_framework(Editor MetalKit)
    add_framework(Editor Foundation)
    add_framework(Editor AVFoundation)
    add_framework(Editor QuartzCore)
    add_framework(Editor Carbon)
    add_framework(Editor IOKit)
    add_framework(Editor ModelIO)
    add_framework(Editor ForceFeedback)
    add_framework(Editor CoreVideo)
    add_framework(Editor AudioToolbox)
    add_framework(Editor CoreAudio)
    add_framework(Editor Cocoa)
    add_framework(Editor GameController)
    add_framework(Editor CoreHaptics)
    add_framework(Editor CoreGraphics)
    target_link_libraries(Editor PRIVATE libRuntime.a SDL2main.a SDL2.a
        iconv)
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_FILES})
SET_WORKING_DIRECTORY(Editor ${CMAKE_SOURCE_DIR})