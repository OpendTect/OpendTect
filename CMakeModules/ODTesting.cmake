#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id$
#_______________________________________________________________________________


set ( OD_TESTDATA_DIR "" CACHE FILEPATH "Test data location" )

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


macro ( OD_ADD_KEYWORD_TEST KW NM )
    if ( NOT DEFINED WIN32 )
	set( CMD "${OpendTect_DIR}/dtect/FindKeyword" )
	list( APPEND CMD "--keyword" "${KW}" "--listfile" "${OD_SOURCELIST_FILE}" )
	set ( EXCEPTIONFILE ${CMAKE_SOURCE_DIR}/CMakeModules/exceptions/${KW}_exceptions )
	if ( EXISTS ${EXCEPTIONFILE} )
	    list( APPEND CMD "--exceptionfile" "${EXCEPTIONFILE}" )
	endif()
	add_test( "Keyword_${NM}" ${CMD} )
    endif()
endmacro()

macro ( OD_ADD_LINEEND_TEST )
    if ( NOT DEFINED WIN32 )
	set( CMD "${OpendTect_DIR}/dtect/FindDosEOL.sh" )
	list( APPEND CMD "${OD_SOURCELIST_FILE}" )
	add_test( LineEndTest ${CMD} )

	set( CMD "${OpendTect_DIR}/dtect/FindNoNewlineAtEndOfFile.csh" )
	list( APPEND CMD "--listfile" "${OD_SOURCELIST_FILE}" )
	add_test( FileEndTest ${CMD} )
 
    endif()
endmacro()


macro ( OD_ADD_SVNPROP_TEST )
    if ( NOT DEFINED WIN32 )
	set( CMD "${OpendTect_DIR}/dtect/CheckSVNProps.csh" )
	list( APPEND CMD "--listfile" "${OD_SOURCELIST_FILE}" )
	add_test( SVNPropertyTest ${CMD} )
    endif()
endmacro()


