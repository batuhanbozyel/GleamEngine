#working directory
macro(SET_WORKING_DIRECTORY TARGET_NAME ROOT_PATH)
	if(WIN32)
		set_target_properties(
    		${TARGET_NAME} PROPERTIES
			VS_DEBUGGER_WORKING_DIRECTORY ${ROOT_PATH})
	else()
		set_target_properties(
			${TARGET_NAME} PROPERTIES
			XCODE_SCHEME_WORKING_DIRECTORY ${ROOT_PATH})
	endif()
endmacro()