#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
#	Jan 2012	K. Tingdahl
#	RCS :		$Id$
#_______________________________________________________________________________

if ( UNIX AND QT_LUPDATE_EXECUTABLE AND (EXISTS ${QT_LUPDATE_EXECUTABLE} ) )
    add_custom_target( update_translations 
		COMMAND ${OpendTect_DIR}/dtect/update_translations.csh
			${OpendTect_DIR} ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR}
			"od"
			${QT_LUPDATE_EXECUTABLE}
		COMMENT "Updating od localizations" )
endif()

