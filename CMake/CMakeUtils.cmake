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

function(GLEAM_SET_WARNINGS TARGET_NAME)
    if (MSVC)
        target_compile_options(${TARGET_NAME} PRIVATE /W4 /WX)
    else()
        target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra -pedantic -Werror -Wno-unused-parameter -Wno-gnu-anonymous-struct -Wno-nested-anon-types)
    endif()
endfunction()

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
    set(oneValueArgs NAME SOURCE_DIR BINARY_DIR DEPENDENCY_DIR)
    set(multiValueArgs HEADERS DEPENDS COMPILER_ARGS)
    cmake_parse_arguments(REFL "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT REFL_DEPENDENCY_DIR)
        set(REFL_DEPENDENCY_DIR ${REFL_BINARY_DIR})
    endif()

    set(REFLECTION_CLI "${GLEAM_ENGINE_DIR}/Tools/CppReflection")
    if (WIN32)
        set(REFLECTION_CLI "${REFLECTION_CLI}.exe")
    endif()

    set(REFLECTION_DATABASE "${REFL_BINARY_DIR}/${REFL_NAME}.Reflection.db")
    set(REFLECTION_HEADER "${REFL_SOURCE_DIR}/${REFL_NAME}.Reflection.generated.h")
    set(REFLECTION_TYPES "${REFL_SOURCE_DIR}/${REFL_NAME}.Reflection.types.inl")

    set(DEPENDENCY_ARGS "")
    set(DEPENDENCY_FILES "")
    foreach(DEPENDENCY ${REFL_DEPENDS})
        list(APPEND DEPENDENCY_ARGS "--dependency=${REFL_DEPENDENCY_DIR}/${DEPENDENCY}.Reflection.db")
        list(APPEND DEPENDENCY_FILES "${REFL_DEPENDENCY_DIR}/${DEPENDENCY}.Reflection.db")
    endforeach()

    add_custom_command(
        OUTPUT ${REFLECTION_DATABASE} ${REFLECTION_HEADER} ${REFLECTION_TYPES}
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
    add_custom_target(${REFL_NAME}Reflection DEPENDS ${REFLECTION_DATABASE} ${REFLECTION_HEADER} ${REFLECTION_TYPES})
    set_target_properties(${REFL_NAME}Reflection PROPERTIES FOLDER CMakeCustomRules)
endfunction()

function(GLEAM_LINK_REFLECTION_MODULES)
    set(oneValueArgs NAME OUTPUT NATVIS)
    set(multiValueArgs DATABASES DEPENDS)
    cmake_parse_arguments(REFL "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(REFLECTION_CLI "${GLEAM_ENGINE_DIR}/Tools/CppReflection")
    if (WIN32)
        set(REFLECTION_CLI "${REFLECTION_CLI}.exe")
    endif()

    set(LINK_OUTPUTS ${REFL_OUTPUT})
    set(LINK_ARGS "--link" "--output=${REFL_OUTPUT}")
    if (REFL_NATVIS)
        list(APPEND LINK_OUTPUTS ${REFL_NATVIS})
        list(APPEND LINK_ARGS "--natvis=${REFL_NATVIS}")
    endif()

    add_custom_command(
        OUTPUT ${LINK_OUTPUTS}
        COMMAND ${REFLECTION_CLI} ${LINK_ARGS} ${REFL_DATABASES}
        DEPENDS ${REFL_DATABASES} ${REFLECTION_CLI}
        COMMENT "Linking reflection databases..."
        COMMAND_EXPAND_LISTS
        VERBATIM
    )
    add_custom_target(${REFL_NAME} DEPENDS ${LINK_OUTPUTS})
    set_target_properties(${REFL_NAME} PROPERTIES FOLDER CMakeCustomRules)
    add_dependencies(${REFL_NAME} ${REFL_DEPENDS})
endfunction()
