# the FindSubversion.cmake module is part of the standard distribution
if ( WIN32 )
    set ( CMAKE_SYSTEM_PROGRAM_PATH ${CMAKE_SYSTEM_PROGRAM_PATH}
	    "C:/Program Files/SlikSvn/bin"
	    "C:/Program Files (x86)/SlikSvn/bin" )
    #Add more likely paths if need be
endif()

find_package( Subversion QUIET )

# extract working copy information for SOURCE_DIR into MY_XXX variables
if ( Subversion_FOUND )
    set ( SUBVERSION_EXEC ${Subversion_SVN_EXECUTABLE} )
else()
    set ( SUBVERSION_EXEC "svn" )
endif()


macro( DEFINE_SVN_EXTERNAL DIR URL REVISION )

    if ( NOT EXISTS ${DIR} )
	execute_process(
	    COMMAND ${SUBVERSION_EXEC} checkout ${EXTRA_SVN_ARGS} -r ${REVISION} ${URL} ${DIR}
	    TIMEOUT 120
	    OUTPUT_VARIABLE OUTPUT
	    ERROR_VARIABLE OUTPUT
	    RESULT_VARIABLE RESULT )
	if ( ${RESULT} EQUAL 0 )
	     message( "${DIR} is updated to ${URL} revision ${REVISION}" )
	else()
	     message( "${DIR} is not up to date:\n${OUTPUT}" )
	endif()
    else()
	if ( UPDATE STREQUAL "Yes" )
	    execute_process(
		COMMAND ${SUBVERSION_EXEC} update ${EXTRA_SVN_ARGS} -r ${REVISION}
		WORKING_DIRECTORY ${DIR}
		TIMEOUT 120
		OUTPUT_VARIABLE OUTPUT
		ERROR_VARIABLE OUTPUT
		RESULT_VARIABLE RESULT )
	    if ( ${RESULT} EQUAL 0 )
		 message( "${DIR} is updated to ${URL} revision ${REVISION}" )
	    else()
		 message( "${DIR} is not up to date:\n${OUTPUT}" )
	    endif()
	endif()
    endif()


endmacro()


find_package( Git QUIET )

if ( Git_FOUND )
    set ( GIT_EXEC ${GIT_EXECUTABLE} )
else()
    set ( GIT_EXEC "git" )
endif()

macro( DEFINE_GIT_EXTERNAL DIR URL BRANCH )

    if ( NOT EXISTS ${OpendTect_DIR}/external/${DIR} )
	execute_process(
	    COMMAND ${GIT_EXEC} clone ${URL} --branch ${BRANCH} --depth 1 ${DIR}
		WORKING_DIRECTORY ${OpendTect_DIR}/external
		OUTPUT_VARIABLE OUTPUT
		ERROR_VARIABLE OUTPUT
		RESULT_VARIABLE RESULT )
	if ( ${RESULT} EQUAL 0 )
	    message ( STATUS "git checkout success for: ${URL}" )
	else()
	    message ( FATAL_ERROR "git checkout failed" )
	endif()
    else()
	execute_process(
	    COMMAND ${GIT_EXEC} pull
	    WORKING_DIRECTORY ${OpendTect_DIR}/external/${DIR}
	    OUTPUT_VARIABLE OUTPUT
	    ERROR_VARIABLE OUTPUT
	    RESULT_VARIABLE RESULT )
	if ( ${RESULT} EQUAL 0 )
	    message ( STATUS "external/${DIR} is updated to ${URL}" )
	else()
	    message ( FATAL_ERROR "${DIR} is not up to date" )
	endif()
    endif()

endmacro()
