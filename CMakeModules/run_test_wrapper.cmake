# run_test_wrapper.cmake
# Pass all arguments directly to the batch file using execute_process()

if ( NOT DEFINED CMD )
    message( FATAL_ERROR "Please provide the command to run in 'CMD'" )
elseif ( NOT EXISTS "${CMD}" )
    message( FATAL_ERROR "Test executable 'CMD' does not exist" )
endif()

set( TEST_ARGS )
if ( QUIET )
    list( APPEND TEST_ARGS "--quiet" )
endif()

if ( "${EXPECTEDRES}" STREQUAL "" )
    set( EXPECTEDRES 0 )
endif()

if ( EXISTS "${PARFILE}" )
    list( APPEND TEST_ARGS "${PARFILE}" )
endif()

if ( IS_DIRECTORY "${DATADIR}" )
    list( APPEND TEST_ARGS --datadir "${DATADIR}" )
endif()

if ( NOT "${PATHDIRS}" STREQUAL "" )
    list( REMOVE_DUPLICATES PATHDIRS )
    if ( WIN32 )
	foreach( DIR ${PATHDIRS} )
	    cmake_path( NATIVE_PATH DIR PATHDIR )
	    list( APPEND ADDEDPATHS "${PATHDIR}" )
	endforeach()
    else()
	set( ADDEDPATHS "${PATHDIRS}" )
    endif()
    set( ENV{PATH} "${ADDEDPATHS};$ENV{PATH}" )
endif()

if ( WIN32 )
    cmake_path( GET CMD PARENT_PATH WORKDIR )
elseif ( UNIX AND IS_DIRECTORY "${LDPATHDIR}" )
    if ( APPLE )
	if ( "$ENV{DYLD_LIBRARY_PATH}" STREQUAL "" )
	    set( ENV{DYLD_LIBRARY_PATH} "${LDPATHDIR}" )
	else()
	    set( ENV{DYLD_LIBRARY_PATH} "${LDPATHDIR};${DYLD_LIBRARY_PATH}" )
	endif()
    else()
	if ( "$ENV{LD_LIBRARY_PATH}" STREQUAL "" )
	    set( ENV{LD_LIBRARY_PATH} "${LDPATHDIR}" )
	else()
	    set( ENV{LD_LIBRARY_PATH} "${LDPATHDIR};${LD_LIBRARY_PATH}" )
	endif()
    endif()
endif()

if ( NOT "${PARENT_CMAKE_BINARY_DIR}" STREQUAL "${OD_BINARY_BASEDIR}" )
    cmake_path( NATIVE_PATH OD_BINARY_BASEDIR DTECT_APPL )
    cmake_path( NATIVE_PATH PARENT_CMAKE_BINARY_DIR OD_USER_PLUGIN_DIR )
    set( ENV{DTECT_APPL} "${DTECT_APPL}" )
    set( ENV{OD_USER_PLUGIN_DIR} "${OD_USER_PLUGIN_DIR}" )
endif()

execute_process(
    COMMAND "${CMD}" ${TEST_ARGS}
    WORKING_DIRECTORY "${WORKDIR}"
    RESULT_VARIABLE TESTRES
)

if ( NOT ${TESTRES} EQUAL ${EXPECTEDRES} )
    message( FATAL_ERROR "Test program ${NAME} returned ${TESTRES}, while ${EXPECTEDRES} was expected" )
endif()
