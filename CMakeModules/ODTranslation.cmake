#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
#	Jan 2012	K. Tingdahl
#	RCS :		$Id$
#_______________________________________________________________________________

if ( QT_LUPDATE_EXECUTABLE AND (EXISTS ${QT_LUPDATE_EXECUTABLE} ) )
endif()

if ( QT_RELEASE_EXECUTABLE AND (EXISTS ${QT_RELEASE_EXECUTABLE} ) )
    file ( GLOB TSFILES RELATIVE ${CMAKE_BINARY_DIR}/data/localizations/
				 ${CMAKE_SOURCE_DIR}/data/localizations/*.ts )

    add_custom_target( localization
		${CMAKE_COMMAND} -E make_directory
			${CMAKE_BINARY_DIR}/data/localizations/
		COMMAND ${QT_RELEASE_EXECUTABLE} ${TSFILES}
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/data/localizations/
		DEPENDS ${TSFILES}
		COMMENT "Building localizations" )

     install( DIRECTORY ${CMAKE_BINARY_DIR}/data/localizations
	      DESTINATION data/localizations
	      FILES_MATCHING PATTERN "*.qm")
endif()
