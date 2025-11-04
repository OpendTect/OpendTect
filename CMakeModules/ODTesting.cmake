#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#


set ( OD_TESTDATA_DIR "" CACHE PATH "Test data location" )

macro ( OD_SETUP_TEST_FILTER )

    if( CTEST_MODEL )
	if ( ${CTEST_MODEL} STREQUAL "Experimental" )
	    set ( CTEST_CUSTOM_TESTS_IGNORE
		${OD_TESTS_IGNORE_EXPERIMENTAL} )
	endif()
	if ( ${CTEST_MODEL} STREQUAL "Continuous" )
	    set ( CTEST_CUSTOM_TESTS_IGNORE
		  ${OD_TESTS_IGNORE_CONTINUOUS} )
	endif()
    endif()

    configure_file (
	"${OpendTect_DIR}/CMakeModules/templates/CTestCustom.cmake.in"
	"${CMAKE_BINARY_DIR}/CTestCustom.cmake" @ONLY )
endmacro()

macro( ADD_RUNTIME_PATHS )
    if ( NOT "${OD_MODULE_RUNTIMEPATH}" STREQUAL "" )
	foreach ( TEST_RUNTIMEPATH ${OD_MODULE_RUNTIMEPATH} )
	    list ( APPEND PATHDIRS "${TEST_RUNTIMEPATH}" )
	endforeach()
    endif()
endmacro( ADD_RUNTIME_PATHS )

macro( ADD_CXX_LD_LIBRARY_PATH )
    get_filename_component( CXXPATH ${CMAKE_CXX_COMPILER} DIRECTORY )
    get_filename_component( CXXPATH ${CXXPATH} DIRECTORY )
    set( LIBSEARCHPATHS "${CXXPATH}/lib64" )
    od_find_library( LIBSTDLOC libstdc++.so.6 )
    if ( LIBSTDLOC )
	get_filename_component( CXXPATH ${LIBSTDLOC} DIRECTORY )
	if ( (NOT "${CXXPATH}" STREQUAL "/usr/lib") AND 
	     (NOT "${CXXPATH}" STREQUAL "/usr/lib64") )
	    set( LDPATHDIR "${CXXPATH}" )
	endif()
    endif()
endmacro( ADD_CXX_LD_LIBRARY_PATH )

macro ( ADD_TEST_PROGRAM TEST_NAME )
    if ( TARGET ${TEST_NAME} )
	set( TEST_CMD "$<TARGET_FILE:${TEST_NAME}>" )
    elseif ( EXISTS "${TEST_NAME}" )
	set( TEST_CMD "${TEST_NAME}" )
    else()
	message( SEND_ERROR "TEST_NAME is neither a target nor the path to an existing executable, cannot add test program" )
    endif()

    if ( WIN32 )
	ADD_RUNTIME_PATHS()
    elseif ( NOT APPLE )
	ADD_CXX_LD_LIBRARY_PATH()
    endif()

    if ( EXISTS "${PARAMETER_FILE}" )
	set( PARFILE "${PARAMETER_FILE}" )
    endif()

    if ( IS_DIRECTORY "${OD_TESTDATA_DIR}" )
	set( DATADIR "${OD_TESTDATA_DIR}" )
    endif()

    add_test( NAME ${TEST_NAME}
	      COMMAND "${CMAKE_COMMAND}"
	      "-DCMD=${TEST_CMD}"
	      "-DNAME=${TEST_NAME}"
	      "-DQUIET=ON"
	      "-DEXPECTEDRES=${TEST_EXPECTEDRES}"
	      "-DPATHDIRS=${PATHDIRS}"
	      "-DLDPATHDIR=${LDPATHDIR}"
	      "-DPARFILE=${PARFILE}"
	      "-DDATADIR=${DATADIR}"
	      "-DOD_BINARY_BASEDIR=${OD_BINARY_BASEDIR}"
	      "-DPARENT_CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}"
	      "-DWORKDIR=${PROJECT_OUTPUT_DIR}"
	      -P "${OpendTect_DIR}/CMakeModules/run_test_wrapper.cmake" )
    unset( PATHDIRS )
    unset( LDPATHDIR )
    unset( PARFILE )
    unset( DATADIR )
    unset( TEST_CMD )

endmacro()

macro( OD_ADD_KEYWORD_TEST KW NM MSG )
    if ( UNIX AND NOT APPLE )
	set( CMD "${OpendTect_DIR}/testscripts/FindKeyword.csh" )
	list( APPEND CMD "--keyword" "\"${KW}\"" "--listfile" "${OD_SOURCELIST_FILE}" )
	set ( EXCEPTIONFILE "${CMAKE_SOURCE_DIR}/CMakeModules/exceptions/${NM}_exceptions" )
	list( APPEND CMD "--grepcommand" "grep" )
	if ( EXISTS "${EXCEPTIONFILE}" )
	    list( APPEND CMD "--exceptionfile" "\"${EXCEPTIONFILE}\"" )
	endif()
	if ( NOT ${MSG} STREQUAL "" )
	    list( APPEND CMD "--message" "\"${MSG}\"" )
	endif()
	if ( POLICY CMP0110 )
	    add_test( "Keyword_${NM}" ${CMD} )
	else()
	    add_test( "\"Keyword_${NM}\"" ${CMD} )
	endif()
    endif()
endmacro()

macro( OD_ADD_REGEXP_TEST KW NM MSG )
    if ( UNIX AND NOT APPLE )
	set( CMD "${OpendTect_DIR}/testscripts/FindKeyword.csh" )
	list( APPEND CMD "--keyword" "\"${KW}\"" "--listfile" "${OD_SOURCELIST_FILE}" )
	list( APPEND CMD "--grepcommand" "egrep" )
	set ( EXCEPTIONFILE "${CMAKE_SOURCE_DIR}/CMakeModules/exceptions/${NM}_exceptions" )
	if ( EXISTS "${EXCEPTIONFILE}" )
	    list( APPEND CMD "--exceptionfile" "\"${EXCEPTIONFILE}\"" )
	endif()
	if ( NOT ${MSG} STREQUAL "" )
	    list( APPEND CMD "--message" "\"${MSG}\"" )
	endif()
	add_test( "Keyword_${NM}" ${CMD} )
    endif()
endmacro()

macro( OD_ADD_LOCAL_STATIC_TEST )
    if ( UNIX AND NOT APPLE )
	set( NM "local_static" )
	set( CMD "${OpendTect_DIR}/testscripts/FindLocalStatic.csh" )
	list( APPEND CMD "--listfile" "${OD_SOURCELIST_FILE}" )
	set ( EXCEPTIONFILE "${CMAKE_SOURCE_DIR}/CMakeModules/exceptions/${NM}_exceptions" )
	if ( EXISTS "${EXCEPTIONFILE}" )
	    list( APPEND CMD "--exceptionfile" "\"${EXCEPTIONFILE}\"" )
	endif()

	add_test( "Keyword_${NM}" ${CMD} )
    endif()
endmacro()

macro( OD_ADD_LINEEND_TEST )
    if ( NOT DEFINED WIN32 )
	set( CMD "${OpendTect_DIR}/testscripts/FindDosEOL.sh" )
	list( APPEND CMD "--listfile" "${OD_SOURCELIST_FILE}" )
	list( APPEND CMD "--wdir" "${CMAKE_BINARY_DIR}" )
	add_test( LineEndTest ${CMD} )

	set( CMD "${OpendTect_DIR}/testscripts/FindNoNewlineAtEndOfFile.sh" )
	list( APPEND CMD "--listfile" "${OD_SOURCELIST_FILE}" )
	list( APPEND CMD "--wdir" "${CMAKE_BINARY_DIR}" )
	add_test( FileEndTest ${CMD} )
    endif()
endmacro()

macro( OD_ADD_LINT_TEST )
    find_program( PHP_PROGRAM "php" )
    if ( PHP_PROGRAM )
	add_test( NAME "PHP_linter"
		  COMMAND "${PHP_PROGRAM}" ./testscripts/test_tab_lint.php --quiet
         	  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" )
    endif()
endmacro()
