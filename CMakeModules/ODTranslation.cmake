#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
#	Jan 2012	K. Tingdahl
#_______________________________________________________________________________

macro( OD_ADD_TRANSLATIONS )

    if ( TARGET Qt${QT_VERSION_MAJOR}::lrelease )
	get_target_property( QT_RELEASE_EXECUTABLE Qt${QT_VERSION_MAJOR}::lupdate
				    IMPORTED_LOCATION )
    endif()

    if ( UNIX AND TARGET Qt${QT_VERSION_MAJOR}::lupdate )
	get_target_property( QT_LUPDATE_EXECUTABLE Qt${QT_VERSION_MAJOR}::lupdate
				    IMPORTED_LOCATION )

	if ( QT_LUPDATE_EXECUTABLE AND EXISTS "${QT_LUPDATE_EXECUTABLE}" )
	    set( LINGUIST_LAUNCHER "CMakeModules/templates/linguist.csh.in" )
	    if ( EXISTS "${LINGUIST_LAUNCHER}" )
		configure_file( "${LINGUIST_LAUNCHER}" dtect/linguist.csh @ONLY )
	    endif()

	    if ( NOT TARGET update_translations )
		add_custom_target( update_translations
			COMMAND ${OpendTect_DIR}/dtect/update_translations.csh
				${OpendTect_DIR} ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR}
				"od"
				"${QT_LUPDATE_EXECUTABLE}"
			COMMENT "Updating plural localizations" )
	    endif()
	endif()
    endif()

endmacro( OD_ADD_TRANSLATIONS )
