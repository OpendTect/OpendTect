#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
#	Jan 2012	K. Tingdahl
#	RCS :		$Id$
#_______________________________________________________________________________

if ( UNIX AND QT_LUPDATE_EXECUTABLE AND (EXISTS ${QT_LUPDATE_EXECUTABLE} ) )
    add_custom_target( update_translations
		${CMAKE_COMMAND} -E make_directory
			${CMAKE_BINARY_DIR}/data/localizations/source
		COMMAND ${CMAKE_SOURCE_DIR}/dtect/update_translations.csh
			${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR}
			${QT_LUPDATE_EXECUTABLE}
		COMMENT "Updating localizations" )
endif()

if ( QT_RELEASE_EXECUTABLE AND (EXISTS ${QT_RELEASE_EXECUTABLE} ) )
    file ( GLOB TSFILES RELATIVE ${CMAKE_BINARY_DIR}/data/localizations/
				 ${CMAKE_SOURCE_DIR}/data/localizations/source/*.ts )

    add_custom_target( localization
		${CMAKE_COMMAND} -E make_directory
			${CMAKE_BINARY_DIR}/data/localizations/
		COMMAND ${QT_RELEASE_EXECUTABLE} ${TSFILES}
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/data/localizations/
		DEPENDS ${TSFILES}
		COMMENT "Releasing localizations" )

     install( DIRECTORY ${CMAKE_BINARY_DIR}/data/localizations
	      DESTINATION ${MISC_INSTALL_PREFIX}/data/localizations
	      FILES_MATCHING PATTERN "*.qm")
endif()
