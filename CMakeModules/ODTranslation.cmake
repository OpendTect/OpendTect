#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#

macro( OD_ADD_TRANSLATIONS )

    if ( UNIX AND TARGET Qt${QT_VERSION_MAJOR}::lupdate )
	get_target_property( QT_LUPDATE_EXECUTABLE Qt${QT_VERSION_MAJOR}::lupdate
			     IMPORTED_LOCATION )

	if ( EXISTS "${QT_LUPDATE_EXECUTABLE}" )
	    if ( NOT TARGET update_translations )
		set( CMAKE_FOLDER "Base" )
		add_custom_target( update_translations
			COMMAND "${OpendTect_DIR}/dtect/update_translations.csh"
				"${OpendTect_DIR}" "${CMAKE_SOURCE_DIR}"
				"${CMAKE_BINARY_DIR}"
				"od"
				"${QT_LUPDATE_EXECUTABLE}"
			COMMENT "Updating plural localizations" )
		unset( CMAKE_FOLDER )
	    endif()
	endif()
    endif()

endmacro( OD_ADD_TRANSLATIONS )
