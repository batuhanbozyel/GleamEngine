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

function(DEPLOY_FILE SRC DST)
    foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
        set(DEPLOY_FILES_DST ${CMAKE_BINARY_DIR}/${OUTPUTCONFIG}/${DST})
        message(STATUS "Copying ${SRC} to ${DEPLOY_FILES_DST}.")
        file(COPY ${SRC} DESTINATION ${DEPLOY_FILES_DST})
    endforeach()
endfunction()