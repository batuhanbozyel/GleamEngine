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

function(DEPLOY_FILE SRC)
    foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
        set(DEPLOY_FILES_DST ${CMAKE_BINARY_DIR}/${OUTPUTCONFIG})
        message(STATUS "Copying ${SRC} to ${DEPLOY_FILES_DST}.")
        file(COPY ${SRC} DESTINATION ${DEPLOY_FILES_DST})
    endforeach()
endfunction()

function(DEPLOY_FILE_TO SRC DST)
    foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
        set(DEPLOY_FILES_DST ${CMAKE_BINARY_DIR}/${OUTPUTCONFIG}/${DST})
        message(STATUS "Copying ${SRC} to ${DEPLOY_FILES_DST}.")
        file(COPY ${SRC} DESTINATION ${DEPLOY_FILES_DST})
    endforeach()
endfunction()

function(GLEAM_ADD_REFLECTION_MODULE)
    set(oneValueArgs NAME SOURCE_DIR BINARY_DIR)
    set(multiValueArgs HEADERS DEPENDS COMPILER_ARGS)
    cmake_parse_arguments(REFL "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(REFLECTION_CLI "${CMAKE_SOURCE_DIR}/Tools/CppReflection")
    if (WIN32)
        set(REFLECTION_CLI "${REFLECTION_CLI}.exe")
    endif()

    set(REFLECTION_DATABASE "${REFL_BINARY_DIR}/${REFL_NAME}.Reflection.db")
    set(REFLECTION_HEADER "${REFL_SOURCE_DIR}/${REFL_NAME}.Reflection.generated.h")

    set(DEPENDENCY_ARGS "")
    set(DEPENDENCY_FILES "")
    foreach(DEPENDENCY ${REFL_DEPENDS})
        list(APPEND DEPENDENCY_ARGS "--dependency=${REFL_BINARY_DIR}/${DEPENDENCY}.Reflection.db")
        list(APPEND DEPENDENCY_FILES "${REFL_BINARY_DIR}/${DEPENDENCY}.Reflection.db")
    endforeach()

    add_custom_command(
        OUTPUT ${REFLECTION_DATABASE} ${REFLECTION_HEADER}
        COMMAND ${REFLECTION_CLI}
            "--module=${REFL_NAME}"
            "--header-dir=${REFL_SOURCE_DIR}"
            "--binary-dir=${REFL_BINARY_DIR}"
            ${DEPENDENCY_ARGS}
            ${REFL_COMPILER_ARGS}
            ${REFL_HEADERS}
        DEPENDS ${REFL_HEADERS} ${REFLECTION_CLI} ${DEPENDENCY_FILES}
        COMMENT "Generating ${REFL_NAME} reflection data from header files..."
        COMMAND_EXPAND_LISTS
        VERBATIM
    )
    add_custom_target(${REFL_NAME}Reflection DEPENDS ${REFLECTION_DATABASE} ${REFLECTION_HEADER})
    set_target_properties(${REFL_NAME}Reflection PROPERTIES FOLDER CMakeCustomRules)
endfunction()

function(GLEAM_LINK_REFLECTION_MODULES)
    set(oneValueArgs OUTPUT BINARY_DIR)
    set(multiValueArgs MODULES)
    cmake_parse_arguments(REFL "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(REFLECTION_CLI "${CMAKE_SOURCE_DIR}/Tools/CppReflection")
    if (WIN32)
        set(REFLECTION_CLI "${REFLECTION_CLI}.exe")
    endif()

    set(MODULE_FILES "")
    set(MODULE_TARGETS "")
    foreach(MODULE ${REFL_MODULES})
        list(APPEND MODULE_FILES "${REFL_BINARY_DIR}/${MODULE}.Reflection.db")
        list(APPEND MODULE_TARGETS "${MODULE}Reflection")
    endforeach()

    add_custom_command(
        OUTPUT ${REFL_OUTPUT}
        COMMAND ${REFLECTION_CLI} "--link" "--output=${REFL_OUTPUT}" ${MODULE_FILES}
        DEPENDS ${MODULE_FILES} ${REFLECTION_CLI}
        COMMENT "Linking reflection databases..."
        COMMAND_EXPAND_LISTS
        VERBATIM
    )
    add_custom_target(ReflectionLink DEPENDS ${REFL_OUTPUT})
    set_target_properties(ReflectionLink PROPERTIES FOLDER CMakeCustomRules)
    add_dependencies(ReflectionLink ${MODULE_TARGETS})
endfunction()
