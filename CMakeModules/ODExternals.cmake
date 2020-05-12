
find_package( Git QUIET )
if ( Git_FOUND )
    set ( GIT_EXEC ${GIT_EXECUTABLE} )
else()
    set ( GIT_EXEC "git" ) # In user-defined path
    execute_process(
	COMMAND ${GIT_EXEC} --version
		RESULT_VARIABLE RESULT
		OUTPUT_VARIABLE GIT_VERSION_STRING
		OUTPUT_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE )
    if ( NOT ${RESULT} EQUAL 0 )
	message ( FATAL_ERROR "git not found: Install it and re-configure." )
    endif()
endif()

if ( ${GIT_VERSION_STRING} LESS "2.2" )
  set( GET_GIT_URL ${GIT_EXEC} ls-remote --get-url )
else()
  set( GET_GIT_URL ${GIT_EXEC} remote get-url origin )
endif()

macro( DEFINE_GIT_EXTERNAL DIR URL BRANCH )

    if ( EXISTS ${CMAKE_SOURCE_DIR}/external/${DIR} )
	# Check URL and Branch of the old checkout
	execute_process(
	    COMMAND ${GET_GIT_URL}
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/external/${DIR}
		RESULT_VARIABLE RESULT
		OUTPUT_VARIABLE OLDURL
		OUTPUT_STRIP_TRAILING_WHITESPACE )
	if ( NOT "${OLDURL}" STREQUAL "${URL}" )
	    message( STATUS "Removing external/${DIR} having URL ${OLDURL}" )
	    file ( REMOVE_RECURSE ${CMAKE_SOURCE_DIR}/external/${DIR} ) 
	else()
	    execute_process(
		COMMAND ${GIT_EXEC} symbolic-ref --short HEAD 
		    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/external/${DIR}
		    OUTPUT_VARIABLE OLDBRANCH
		    RESULT_VARIABLE RESULT
		    OUTPUT_STRIP_TRAILING_WHITESPACE )
	    if ( NOT ${OLDBRANCH} STREQUAL ${BRANCH} )
		message( STATUS "Removing external/${DIR} branch ${OLDBRANCH}" )
		file ( REMOVE_RECURSE ${CMAKE_SOURCE_DIR}/external/${DIR} ) 
	    endif()
	endif() 
    endif()

    if ( NOT EXISTS ${CMAKE_SOURCE_DIR}/external/${DIR} )
	execute_process(
	    COMMAND ${GIT_EXEC} clone ${URL} --branch ${BRANCH} --depth 1 ${DIR}
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/external
		OUTPUT_VARIABLE OUTPUT
		ERROR_VARIABLE OUTPUT
		RESULT_VARIABLE RESULT )
	if ( ${RESULT} EQUAL 0 )
	    message( STATUS "git checkout success for: ${URL}, branch ${BRANCH}" )
	else()
	    message( SEND_ERROR "git cmd=${GIT_EXEC} clone ${URL} --branch ${BRANCH} --depth 1 ${DIR}" )
	    message( SEND_ERROR "git workdir=${CMAKE_SOURCE_DIR}/external" )
	    message( FATAL_ERROR "git checkout failed" )
	endif()
    else()
	execute_process(
	    COMMAND ${GIT_EXEC} pull
	    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/external/${DIR}
	    OUTPUT_VARIABLE OUTPUT
	    ERROR_VARIABLE OUTPUT
	    RESULT_VARIABLE RESULT )
	if ( ${RESULT} EQUAL 0 )
	    message ( STATUS "external/${DIR} is updated" )
	else()
	    message ( FATAL_ERROR "${DIR} is not up to date" )
	endif()
    endif()

endmacro()
