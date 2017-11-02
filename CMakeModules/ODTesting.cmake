#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id$
#_______________________________________________________________________________


set ( OD_TESTDATA_DIR "" CACHE FILEPATH "Test data location" )

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

macro ( ADD_TEST_PROGRAM TEST_NAME )
    if ( WIN32 )
	set ( TEST_COMMAND "${OpendTect_DIR}/testscripts/run_test.cmd" )
	set ( TEST_ARGS --command ${TEST_NAME}.exe )
    else()
	set ( TEST_COMMAND "${OpendTect_DIR}/testscripts/run_test.csh" )
	set ( TEST_ARGS --command ${TEST_NAME} )
    endif()

    list ( APPEND TEST_ARGS --wdir ${CMAKE_BINARY_DIR}
		    --config ${CMAKE_BUILD_TYPE} --plf ${OD_PLFSUBDIR}
		    --qtdir ${QTDIR}
		    --quiet )

    if ( EXISTS ${PARAMETER_FILE} )
	list( APPEND TEST_ARGS --parfile ${PARAMETER_FILE} )
    endif()

    if ( NOT (OD_TESTDATA_DIR STREQUAL "") )
	if ( EXISTS ${OD_TESTDATA_DIR} )
	    list ( APPEND TEST_ARGS --datadir ${OD_TESTDATA_DIR} )
	endif()
    endif()

    add_test( NAME ${TEST_NAME} WORKING_DIRECTORY ${OD_EXEC_OUTPUT_PATH}
	      COMMAND ${TEST_COMMAND} ${TEST_ARGS} )

    if ( UNIX AND VALGRIND_PROGRAM )
	add_test( NAME ${TEST_NAME}_memcheck WORKING_DIRECTORY ${OD_EXEC_OUTPUT_PATH}
	      COMMAND ${TEST_COMMAND} ${TEST_ARGS} --valgrind ${VALGRIND_PROGRAM} )
    endif()

endmacro()



macro ( OD_ADD_KEYWORD_TEST KW NM MSG)
    if ( (NOT DEFINED WIN32) AND (NOT DEFINED APPLE) )
	set( CMD "${OpendTect_DIR}/testscripts/FindKeyword.csh" )
	list( APPEND CMD "--keyword" "${KW}" "--listfile" "${OD_SOURCELIST_FILE}" )
	set ( EXCEPTIONFILE ${CMAKE_SOURCE_DIR}/CMakeModules/exceptions/${NM}_exceptions )
	list( APPEND CMD "--grepcommand" "grep" )
	if ( EXISTS ${EXCEPTIONFILE} )
	    list( APPEND CMD "--exceptionfile" "${EXCEPTIONFILE}" )
	endif()
	if ( NOT (${MSG} STREQUAL "" ) )
	    list( APPEND CMD "--message" ${MSG} )
	endif()
	add_test( "Keyword_${NM}" ${CMD} )
    endif()
endmacro()

macro ( OD_ADD_REGEXP_TEST KW NM MSG)
    if ( (NOT DEFINED WIN32) AND (NOT DEFINED APPLE) )
	set( CMD "${OpendTect_DIR}/testscripts/FindKeyword.csh" )
	list( APPEND CMD "--keyword" "${KW}" "--listfile" "${OD_SOURCELIST_FILE}" )
	list( APPEND CMD "--grepcommand" "egrep" )
	set ( EXCEPTIONFILE ${CMAKE_SOURCE_DIR}/CMakeModules/exceptions/${NM}_exceptions )
	if ( EXISTS ${EXCEPTIONFILE} )
	    list( APPEND CMD "--exceptionfile" "${EXCEPTIONFILE}" )
	endif()
	if ( NOT (${MSG} STREQUAL "" ) )
	    list( APPEND CMD "--message" ${MSG} )
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
	list( APPEND CMD "${OD_SOURCELIST_FILE}" )
	list( APPEND CMD "--wdir" ${CMAKE_SOURCE_DIR} )
	add_test( LineEndTest ${CMD} )

	set( CMD "${OpendTect_DIR}/testscripts/FindNoNewlineAtEndOfFile.csh" )
	list( APPEND CMD "--listfile" "${OD_SOURCELIST_FILE}" )
	add_test( FileEndTest ${CMD} )
 
    endif()
endmacro()


macro ( OD_ADD_EXIT_PROGRAM_TEST )
    if ( WIN32 )
	set( CMD "${OpendTect_DIR}/testscripts/run_test.cmd" )
	list ( APPEND CMD --command test_exit_program.exe )
    else()
	set( CMD "${OpendTect_DIR}/testscripts/run_test.csh" )
	list ( APPEND CMD --command test_exit_program )
    endif()

    list ( APPEND CMD --wdir ${CMAKE_BINARY_DIR}
                    --config ${CMAKE_BUILD_TYPE} --plf ${OD_PLFSUBDIR}
                    --qtdir ${QTDIR}
		    --expected-result 1
                    --quiet )

    add_test( test_exit_program ${CMD} )
endmacro()
