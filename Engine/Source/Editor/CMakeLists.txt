include (${CMAKE_SOURCE_DIR}/CMake/CMakeUtils.cmake)

set(INCLUDE_DIRS_EDITOR
    src
    ${CMAKE_SOURCE_DIR}/Engine/Source/Runtime/src
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/SDL2/include
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/fmt/include
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/entt/single_include
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/imgui
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/ImGuizmo
)

file(GLOB_RECURSE SOURCE_FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.mm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
)

add_executable(Editor WIN32 MACOSX_BUNDLE ${SOURCE_FILES})
add_dependencies(Editor SDL2main SDL2-static Runtime imgui)
target_include_directories(Editor PRIVATE ${INCLUDE_DIRS_EDITOR})

target_link_directories(Editor PRIVATE ${CMAKE_SOURCE_DIR}/bin/$<CONFIG>)
if (WIN32)
    target_link_libraries(Editor PRIVATE Runtime.lib SDL2main.lib SDL2-static.lib imgui.lib
        winmm
        version
        Imm32
        Setupapi)
else()
    SET(MACOS_FRAMEWORKS   "-framework Cocoa
                            -framework Carbon
                            -framework IOKit
                            -framework ModelIO
                            -framework ForceFeedback
                            -framework AudioToolbox
                            -framework Metal
                            -framework MetalKit
                            -framework Foundation
                            -framework AVFoundation
                            -framework CoreVideo
                            -framework CoreAudio
                            -framework CoreGraphics
                            -framework QuartzCore
                            -framework GameController
                            -framework CoreHaptics
                            -framework UniformTypeIdentifiers")
    target_link_libraries(Editor PRIVATE ${MACOS_FRAMEWORKS} Runtime.a SDL2main.a SDL2.a imgui.a iconv)

    set_source_files_properties(${SOURCE_FILES} PROPERTIES COMPILE_FLAGS "-x objective-c++")
    set_target_properties(Editor PROPERTIES MACOSX_BUNDLE YES XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES)
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_FILES})
SET_WORKING_DIRECTORY(Editor ${CMAKE_SOURCE_DIR})
