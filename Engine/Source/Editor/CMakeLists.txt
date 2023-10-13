include (${CMAKE_SOURCE_DIR}/CMake/CMakeUtils.cmake)

find_package(Vulkan REQUIRED)

set(INCLUDE_DIRS
    src
    ${CMAKE_SOURCE_DIR}/Engine/Source/Runtime/src
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/SDL/include
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/fmt/include
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/entt/single_include
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/tinyobjloader
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/OpenFBX/src
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/cgltf
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/imgui
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/ImGuizmo
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/volk 
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/VulkanMemoryAllocator/include
)

file(GLOB_RECURSE SOURCE_FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.mm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
)

add_executable(Editor WIN32 MACOSX_BUNDLE ${SOURCE_FILES})
target_include_directories(Editor PRIVATE
    ${INCLUDE_DIRS} 
    ${Vulkan_INCLUDE_DIRS}
)
target_link_directories(Editor PRIVATE ${CMAKE_SOURCE_DIR}/bin/$<CONFIG>)

# Add imgui as static library
add_library(imgui STATIC
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/imgui/imgui.cpp
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/imgui/imgui_draw.cpp
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/imgui/imgui_widgets.cpp
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/imgui/imgui_demo.cpp
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/imgui/imgui_tables.cpp
)

# Configure imgui
target_include_directories(imgui PRIVATE
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/imgui
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/SDL/include
    ${Vulkan_INCLUDE_DIRS}
)

if (APPLE)
    set_target_properties(Editor PROPERTIES MACOSX_BUNDLE YES XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES)
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_FILES})
SET_WORKING_DIRECTORY(Editor ${CMAKE_SOURCE_DIR})

# Link Runtime and imgui
get_target_property(GLEAM_RUNTIME_LIBS Runtime LINK_LIBRARIES)
if (WIN32)
    target_link_libraries(Editor PRIVATE Runtime.lib imgui.lib ${GLEAM_RUNTIME_LIBS})
else()
    target_link_libraries(Editor PRIVATE Runtime.a imgui.a ${GLEAM_RUNTIME_LIBS})
endif()

add_dependencies(Editor Runtime imgui)

target_compile_definitions(
    Editor

    PRIVATE
    "_CRT_SECURE_NO_WARNINGS"
    "NOMINMAX"
)
