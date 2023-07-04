#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#

find_package( Git QUIET )
if ( Git_FOUND )
    set ( GIT_EXEC ${GIT_EXECUTABLE} )
else()
    set ( GIT_EXEC "git" ) # In user-defined path
    execute_process(
	COMMAND ${GIT_EXEC} --version
		RESULT_VARIABLE STATUS
		OUTPUT_VARIABLE GIT_VERSION_STRING
		OUTPUT_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE )
    if ( NOT ${STATUS} EQUAL 0 )
	message ( FATAL_ERROR "git not found: Install it and re-configure." )
    endif()
endif()

if ( ${GIT_VERSION_STRING} LESS "2.2" )
  set( GET_GIT_URL ${GIT_EXEC} ls-remote --get-url )
else()
  set( GET_GIT_URL ${GIT_EXEC} remote get-url origin )
endif()

macro( DEFINE_GIT_EXTERNAL DIR URL_STR BRANCH )
    set( ISTAG FALSE )
    set( branchtag "branch" )
    set( extra_args ${ARGN} )
    foreach( optional_arg ${extra_args} )
	if ( ${optional_arg} STREQUAL "TAG" )
	    set( ISTAG TRUE )
	    set( branchtag "tag" )
	    break()
	endif()
    endforeach()
    SET( URL ${URL_STR} )
    execute_process(
	COMMAND ${GET_GIT_URL}
	    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	    RESULT_VARIABLE STATUS
	    OUTPUT_VARIABLE MAIN_URL
	    OUTPUT_STRIP_TRAILING_WHITESPACE )
    STRING( FIND "${MAIN_URL}" "git@github.com" USE_SSH_URL )
    if( NOT "${USE_SSH_URL}" EQUAL -1 )
	STRING( REPLACE "https://github.com/" "git@github.com:" NEWURL ${URL} )
	#message( "Using SSH URL for external ${NEWURL}" )
	SET( URL ${NEWURL} )
    endif()

    if ( IS_DIRECTORY "${CMAKE_SOURCE_DIR}/external/${DIR}" )
	# Check URL and Branch of the old checkout
	execute_process(
	    COMMAND ${GET_GIT_URL}
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/external/${DIR}
		RESULT_VARIABLE STATUS
		OUTPUT_VARIABLE OLDURL
		OUTPUT_STRIP_TRAILING_WHITESPACE )
	if ( NOT ${OLDURL} STREQUAL ${URL} )
	    message( STATUS "Removing external/${DIR} having URL ${OLDURL}" )
	    file ( REMOVE_RECURSE ${CMAKE_SOURCE_DIR}/external/${DIR} ) 
	    if ( IS_DIRECTORY "${OD_BINARY_BASEDIR}/external/${DIR}" )
		file ( REMOVE_RECURSE "${OD_BINARY_BASEDIR}/external/${DIR}" )
	    endif()
	else()
	    execute_process(
		COMMAND ${GIT_EXEC} symbolic-ref --short HEAD 
		    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/external/${DIR}
		    OUTPUT_VARIABLE OLDBRANCH
		    ERROR_QUIET
		    RESULT_VARIABLE STATUS
		    OUTPUT_STRIP_TRAILING_WHITESPACE )
	    if ( ${STATUS} EQUAL 0 AND ISTAG )
		STRING( REPLACE "heads/" "" OLDBRANCH ${OLDBRANCH} )
	    endif()
	    if ( NOT ${STATUS} EQUAL 0 OR NOT ${OLDBRANCH} STREQUAL ${BRANCH} )
		message( STATUS "Removing external/${DIR} ${branchtag} ${OLDBRANCH}" )
		file ( REMOVE_RECURSE "${CMAKE_SOURCE_DIR}/external/${DIR}" )
		if ( IS_DIRECTORY "${OD_BINARY_BASEDIR}/external/${DIR}" )
		    file ( REMOVE_RECURSE "${OD_BINARY_BASEDIR}/external/${DIR}" )
		endif()
	    endif()
	endif() 
    endif()

    if ( NOT IS_DIRECTORY "${CMAKE_SOURCE_DIR}/external/${DIR}" )
	execute_process(
	    COMMAND ${GIT_EXEC} clone ${URL} --branch ${BRANCH} --depth 1 ${DIR}
		WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/external"
		OUTPUT_VARIABLE OUTPUT
		ERROR_VARIABLE OUTPUT
		RESULT_VARIABLE STATUS )
	if ( ${STATUS} EQUAL 0 )
	    message(STATUS "git checkout success for: ${URL}, ${branchtag} ${BRANCH}")
	else()
	    message( SEND_ERROR "git cmd=${GIT_EXEC} clone ${URL} --branch ${BRANCH} --depth 1 ${DIR}" )
	    message( SEND_ERROR "git workdir=${CMAKE_SOURCE_DIR}/external" )
	    message( FATAL_ERROR "git checkout failed" )
	endif()
	if ( ISTAG )
	    execute_process(
		COMMAND ${GIT_EXEC} checkout -b ${BRANCH}
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/external/${DIR}
		OUTPUT_QUIET
		ERROR_QUIET
	    )
	endif()
    else()
	execute_process(
	    COMMAND ${GIT_EXEC} pull
	    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/external/${DIR}"
	    OUTPUT_VARIABLE OUTPUT
	    ERROR_VARIABLE OUTPUT
	    RESULT_VARIABLE STATUS )
	if ( ${STATUS} EQUAL 0 )
	    if ( NOT OUTPUT MATCHES ".*Already up to date.*" )
		message ( STATUS "external/${DIR} is updated" )
		if ( DEFINED OD_BINARY_BASEDIR )
		    set( CACHEFILE "${OD_BINARY_BASEDIR}" )
		else()
		    set( CACHEFILE "${CMAKE_BINARY_DIR}" )
		endif()
		set( CACHEFILE "${CACHEFILE}/external/${DIR}/CMakeCache.txt" )

		if ( EXISTS "${CACHEFILE}" )
		    file(REMOVE "${CACHEFILE}")
		endif()
		file(TOUCH_NOCREATE "${CMAKE_BINARY_DIR}/CMakeCache.txt")
	    endif()
	else()
	    message ( WARNING "Cannot check status for '${DIR}' external. It may not be up-to-date" )
	endif()
    endif()

endmacro()
