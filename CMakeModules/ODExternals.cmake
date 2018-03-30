# the FindSubversion.cmake module is part of the standard distribution
if ( WIN32 )
    set ( CMAKE_SYSTEM_PROGRAM_PATH ${CMAKE_SYSTEM_PROGRAM_PATH}
	    "C:/Program Files/SlikSvn/bin"
	    "C:/Program Files (x86)/SlikSvn/bin" )
    #Add more likely paths if need be
endif()

find_package( Subversion QUIET )
if ( Subversion_FOUND )
    set ( SUBVERSION_EXEC ${Subversion_SVN_EXECUTABLE} )
else()
    set ( SUBVERSION_EXEC "svn" ) # In user-defined path
endif()
execute_process(
    COMMAND ${SUBVERSION_EXEC} --version
	RESULT_VARIABLE RESULT OUTPUT_QUIET
)
if ( NOT ${RESULT} EQUAL 0 )
    message ( FATAL_ERROR "svn not found: Install it and re-configure." )
endif()

# EXTBASEDIR: Default: ${CMAKE_SOURCE_DIR}/external
macro( DEFINE_SVN_EXTERNAL DIR URL EXTBASEDIR REVISION )

    if ( NOT EXISTS ${EXTBASEDIR}/${DIR} )
	execute_process(
	    COMMAND ${SUBVERSION_EXEC} checkout ${EXTRA_SVN_ARGS} -r ${REVISION} ${URL} ${EXTBASEDIR}/${DIR}
	    TIMEOUT 600
	    OUTPUT_VARIABLE OUTPUT
	    ERROR_VARIABLE OUTPUT
	    RESULT_VARIABLE RESULT )
	if ( ${RESULT} EQUAL 0 )
	     message( STATUS "svn checkout success for: ${URL} revision ${REVISION}" )
	else()
	     message( FATAL_ERROR "svn checkout failed:\n${OUTPUT}" )
	endif()
    else()
	execute_process(
	    COMMAND ${SUBVERSION_EXEC} update ${EXTRA_SVN_ARGS} -r ${REVISION}
	    WORKING_DIRECTORY ${EXTBASEDIR}/${DIR}
	    TIMEOUT 600
	    OUTPUT_VARIABLE OUTPUT
	    ERROR_VARIABLE OUTPUT
	    RESULT_VARIABLE RESULT )
	if ( ${RESULT} EQUAL 0 )
	     message ( STATUS "${EXTBASEDIR}/${DIR} is updated" )
	else()
	     message ( FATAL_ERROR "${DIR} is not up to date:\n${OUTPUT}" )
	endif()
    endif()

endmacro()


find_package( Git QUIET )
if ( Git_FOUND )
    set ( GIT_EXEC ${GIT_EXECUTABLE} )
else()
    set ( GIT_EXEC "git" ) # In user-defined path
endif()
execute_process(
    COMMAND ${GIT_EXEC} --version
	    RESULT_VARIABLE RESULT OUTPUT_QUIET
)
if ( NOT ${RESULT} EQUAL 0 )
    message ( FATAL_ERROR "git not found: Install it and re-configure." )
endif()

macro( DEFINE_GIT_EXTERNAL DIR URL BRANCH )

    if ( NOT EXISTS ${CMAKE_SOURCE_DIR}/external/${DIR} )
	execute_process(
	    COMMAND ${GIT_EXEC} clone ${URL} --branch ${BRANCH} --depth 1 ${DIR}
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/external
		OUTPUT_VARIABLE OUTPUT
		ERROR_VARIABLE OUTPUT
		RESULT_VARIABLE RESULT )
	if ( ${RESULT} EQUAL 0 )
	    message ( STATUS "git checkout success for: ${URL}" )
	else()
	    message( "git cmd=${GIT_EXEC} clone ${URL} --branch ${BRANCH} --depth 1 ${DIR}" )
	    message( "git workdir=${CMAKE_SOURCE_DIR}/external" )
	    message ( FATAL_ERROR "git checkout failed" )
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
