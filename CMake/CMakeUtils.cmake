#working directory
MACRO(SET_WORKING_DIRECTORY TARGET_NAME GIVEN_PATH)
	if(WIN32)
		set_target_properties(
    		${TARGET_NAME} PROPERTIES
			VS_DEBUGGER_WORKING_DIRECTORY ${GIVEN_PATH})
	else()
		set_target_properties(
			${TARGET_NAME} PROPERTIES
			XCODE_SCHEME_WORKING_DIRECTORY ${GIVEN_PATH})
	endif()
ENDMACRO(SET_WORKING_DIRECTORY)

macro(ADD_GRAPHICS_SHADER target file vertexEntry fragmentEntry)
    add_custom_command(
        TARGET ${target}
        PRE_BUILD
        COMMAND $ENV{VULKAN_SDK}/bin/dxc -spirv -fvk-invert-y -T vs_6_0 -E ${vertexEntry} ${file} -Fo ${CMAKE_SOURCE_DIR}/Assets/${vertexEntry}.spv
        COMMAND $ENV{VULKAN_SDK}/bin/dxc -spirv -T ps_6_0 -E ${fragmentEntry} ${file} -Fo ${CMAKE_SOURCE_DIR}/Assets/${fragmentEntry}.spv
    )
endmacro(ADD_GRAPHICS_SHADER)

macro(ADD_COMPUTE_SHADER target file entry)
    add_custom_command(
        TARGET ${target}
        PRE_BUILD
        COMMAND $ENV{VULKAN_SDK}/bin/dxc -spirv -fvk-use-scalar-layout -T cs_6_0 -E ${entry} ${file} -Fo ${CMAKE_SOURCE_DIR}/Assets/${entry}.spv
    )
endmacro(ADD_COMPUTE_SHADER)