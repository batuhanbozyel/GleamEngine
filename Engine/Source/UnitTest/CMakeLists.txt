include (${CMAKE_SOURCE_DIR}/CMake/CMakeUtils.cmake)

# GoogleTest library target
set(INCLUDE_DIRS_googletest
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/googletest/googletest
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/googletest/googletest/include
)

file(GLOB_RECURSE SOURCE_FILES_googletest
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/googletest/googletest/src/gtest-all.cc
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/googletest/googletest/src/gtest_main.cc
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/googletest/googletest/include/*.h
)
add_library(googletest STATIC ${SOURCE_FILES_googletest})
target_include_directories(googletest PRIVATE ${INCLUDE_DIRS_googletest})
SET_WORKING_DIRECTORY(googletest ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/googletest/googletest)

# UnitText executable target
set(INCLUDE_DIRS_UNIT_TEST
    src
    ${CMAKE_SOURCE_DIR}/Engine/Source/Runtime/src
    ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/googletest/googletest/include
)

file(GLOB_RECURSE SOURCE_FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
)

add_executable(UnitTest ${SOURCE_FILES})
add_dependencies(UnitTest googletest)
target_include_directories(UnitTest PRIVATE ${INCLUDE_DIRS_UNIT_TEST})
target_link_directories(UnitTest PRIVATE ${CMAKE_SOURCE_DIR}/bin/$<CONFIG>)
if (WIN32)
    target_link_libraries(UnitTest PRIVATE googletest.lib)
else()
    target_link_libraries(UnitTest PRIVATE googletest.a)
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_FILES})
SET_WORKING_DIRECTORY(UnitTest ${CMAKE_SOURCE_DIR})