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