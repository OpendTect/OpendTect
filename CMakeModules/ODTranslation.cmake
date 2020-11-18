#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
#	Jan 2012	K. Tingdahl
#_______________________________________________________________________________

if ( TARGET Qt5::lrelease )
    get_target_property( QT_RELEASE_EXECUTABLE Qt5::lupdate
				IMPORTED_LOCATION )
endif()

if ( UNIX AND TARGET Qt5::lupdate )
    get_target_property( QT_LUPDATE_EXECUTABLE Qt5::lupdate
				IMPORTED_LOCATION )

    if ( QT_LUPDATE_EXECUTABLE AND EXISTS "${QT_LUPDATE_EXECUTABLE}" )
	set( LINGUIST_LAUNCHER "CMakeModules/templates/linguist.csh.in" )
	if ( EXISTS "${LINGUIST_LAUNCHER}" )
	    configure_file( "${LINGUIST_LAUNCHER}" dtect/linguist.csh @ONLY )
	endif()

	add_custom_target( update_translations
		COMMAND ${OpendTect_DIR}/dtect/update_translations.csh
			${OpendTect_DIR} ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR}
			"od"
			"${QT_LUPDATE_EXECUTABLE}"
		COMMENT "Updating plural localizations" )
    endif()
endif()
