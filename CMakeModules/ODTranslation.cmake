#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
#	Jan 2012	K. Tingdahl
#_______________________________________________________________________________

if ( UNIX AND QT_LUPDATE_EXECUTABLE AND (EXISTS ${QT_LUPDATE_EXECUTABLE} ) )
    add_custom_target( update_translations  ALL
		COMMAND ${OpendTect_DIR}/dtect/update_translations.sh
			${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR}
			${OD_SUBSYSTEM}
			${QT_LUPDATE_EXECUTABLE}
			--quiet
		COMMENT "Updating od localizations" )
endif()
