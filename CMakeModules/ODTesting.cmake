#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#


set ( OD_TESTDATA_DIR "" CACHE PATH "Test data location" )


if ( UNIX )
    set ( VALGRIND_PROGRAM "" CACHE PATH "Location of valgrind" )
endif()

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
	${OpendTect_DIR}/CMakeModules/templates/CTestCustom.cmake.in
	${CMAKE_SOURCE_DIR}/CTestCustom.cmake @ONLY )
endmacro()

macro( ADD_RUNTIME_PATHS )
    if ( NOT "${OD_MODULE_RUNTIMEPATH}" STREQUAL "" )
	foreach ( TEST_RUNTIMEPATH ${OD_MODULE_RUNTIMEPATH} )
	    list ( APPEND TEST_ARGS --pathdirs "${TEST_RUNTIMEPATH}" )
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
	    list ( APPEND TEST_ARGS --ldpathdir ${CXXPATH} )
	endif()
    endif()
endmacro( ADD_CXX_LD_LIBRARY_PATH )

macro ( ADD_TEST_PROGRAM TEST_NAME )
    if ( WIN32 )
	set ( TEST_COMMAND "${OpendTect_DIR}/testscripts/run_test.cmd" )
    else()
	set ( TEST_COMMAND "${OpendTect_DIR}/testscripts/run_test.csh" )
    endif()
    set ( TEST_ARGS --command ${TEST_NAME} )

    list ( APPEND TEST_ARGS --wdir "${PROJECT_OUTPUT_DIR}"
		    --config ${CMAKE_BUILD_TYPE} --plf ${OD_PLFSUBDIR}
		    --quiet )
    if ( WIN32 )
	ADD_RUNTIME_PATHS()
    else()
	list ( APPEND TEST_ARGS
		    --oddir "${OD_BINARY_BASEDIR}" )
	if ( NOT APPLE )
	    ADD_CXX_LD_LIBRARY_PATH()
	endif()
    endif()

    if ( EXISTS ${PARAMETER_FILE} )
	list( APPEND TEST_ARGS --parfile ${PARAMETER_FILE} )
    endif()

    if ( NOT (OD_TESTDATA_DIR STREQUAL "") )
	if ( EXISTS ${OD_TESTDATA_DIR} )
	    list ( APPEND TEST_ARGS --datadir ${OD_TESTDATA_DIR} )
	endif()
    endif()

    add_test( NAME ${TEST_NAME} WORKING_DIRECTORY "${PROJECT_OUTPUT_DIR}"
	      COMMAND ${TEST_COMMAND} ${TEST_ARGS} )

    if ( UNIX AND VALGRIND_PROGRAM )
	add_test( NAME ${TEST_NAME}_memcheck WORKING_DIRECTORY "${PROJECT_OUTPUT_DIR}"
	      COMMAND ${TEST_COMMAND} ${TEST_ARGS} --valgrind ${VALGRIND_PROGRAM} )
    endif()

endmacro()



macro ( OD_ADD_KEYWORD_TEST KW NM MSG)
    if ( (NOT DEFINED WIN32) AND (NOT DEFINED APPLE) )
	set( CMD "${OpendTect_DIR}/testscripts/FindKeyword.csh" )
	list( APPEND CMD "--keyword" "\"${KW}\"" "--listfile" "${OD_SOURCELIST_FILE}" )
	set ( EXCEPTIONFILE ${CMAKE_SOURCE_DIR}/CMakeModules/exceptions/${NM}_exceptions )
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

macro ( OD_ADD_REGEXP_TEST KW NM MSG)
    if ( (NOT DEFINED WIN32) AND (NOT DEFINED APPLE) )
	set( CMD "${OpendTect_DIR}/testscripts/FindKeyword.csh" )
	list( APPEND CMD "--keyword" "\"${KW}\"" "--listfile" "${OD_SOURCELIST_FILE}" )
	list( APPEND CMD "--grepcommand" "egrep" )
	set ( EXCEPTIONFILE ${CMAKE_SOURCE_DIR}/CMakeModules/exceptions/${NM}_exceptions )
	if ( EXISTS "${EXCEPTIONFILE}" )
	    list( APPEND CMD "--exceptionfile" "\"${EXCEPTIONFILE}\"" )
	endif()
	if ( NOT ${MSG} STREQUAL "" )
	    list( APPEND CMD "--message" "\"${MSG}\"" )
	endif()
	add_test( "Keyword_${NM}" ${CMD} )
    endif()
endmacro()


macro ( OD_ADD_LOCAL_STATIC_TEST )
    if ( (NOT DEFINED WIN32) AND (NOT DEFINED APPLE) )
	set( NM "local_static" )
	set( CMD "${OpendTect_DIR}/testscripts/FindLocalStatic.csh" )
	list( APPEND CMD "--listfile" "${OD_SOURCELIST_FILE}" )
	set ( EXCEPTIONFILE ${CMAKE_SOURCE_DIR}/CMakeModules/exceptions/${NM}_exceptions )
	if ( EXISTS ${EXCEPTIONFILE} )
	    list( APPEND CMD "--exceptionfile" "${EXCEPTIONFILE}" )
	endif()

	add_test( "Keyword_${NM}" ${CMD} )
    endif()
endmacro()

macro ( OD_ADD_LINEEND_TEST )
    if ( NOT DEFINED WIN32 )
	set( CMD "${OpendTect_DIR}/testscripts/FindDosEOL.sh" )
	list( APPEND CMD "--listfile" "${OD_SOURCELIST_FILE}" )
	list( APPEND CMD "--wdir" ${CMAKE_BINARY_DIR} )
	add_test( LineEndTest ${CMD} )

	set( CMD "${OpendTect_DIR}/testscripts/FindNoNewlineAtEndOfFile.sh" )
	list( APPEND CMD "--listfile" "${OD_SOURCELIST_FILE}" )
	list( APPEND CMD "--wdir" ${CMAKE_BINARY_DIR} )
	add_test( FileEndTest ${CMD} )
 
    endif()
endmacro()


macro ( OD_ADD_EXIT_PROGRAM_TEST )
    set ( APPEND TEST_ARGS --command test_exit_program )
    if ( WIN32 )
	set( TEST_COMMAND "${OpendTect_DIR}/testscripts/run_test.cmd" )
    else()
	set( TEST_COMMAND "${OpendTect_DIR}/testscripts/run_test.csh" )
    endif()
    set ( TEST_ARGS --command test_exit_program )

    list ( APPEND TEST_ARGS --wdir "${OD_BINARY_BASEDIR}"
		    --config ${CMAKE_BUILD_TYPE} --plf ${OD_PLFSUBDIR}
		    --expected-result 1
		    --quiet )
    set( OD_MODULE_RUNTIMEPATH "${QTDIR}/bin" )
    if ( WIN32 )
	ADD_RUNTIME_PATHS()
    else()
	list ( APPEND TEST_ARGS
		    --oddir ${OD_BINARY_BASEDIR} )
	if ( NOT APPLE )
	    ADD_CXX_LD_LIBRARY_PATH()
	endif()
    endif()

    add_test( test_exit_program "${TEST_COMMAND}" ${TEST_ARGS}  )
endmacro()

macro ( OD_ADD_LINT_TEST )
    find_program( PHP_PROGRAM "php" )
    if ( PHP_PROGRAM )
	add_test( NAME "PHP_linter" COMMAND ${PHP_PROGRAM} ./testscripts/test_tab_lint.php --quiet
         	  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" )
    endif()
endmacro()
