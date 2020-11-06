#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#_______________________________________________________________________________

MACRO( OD_SETUP_ZLIB )
    if ( NOT DEFINED ZLIB_INCLUDE_DIR )
	if ( Qt5Core_FOUND )
	    find_package( Qt5 QUIET COMPONENTS Zlib )
	    if ( Qt5Zlib_FOUND )
		set( ZLIB_LIBRARY Qt5::Core )
		get_target_property( ZLIB_INCLUDE_DIR Qt5::Zlib INTERFACE_INCLUDE_DIRECTORIES )
	    endif()
	elseif ( QT_QTCORE_INCLUDE_DIR )
	    if ( EXISTS "${QT_QTCORE_INCLUDE_DIR}/zlib.h" )
		set ( ZLIB_INCLUDE_DIR ${QT_QTCORE_INCLUDE_DIR} )
		set ( ZLIB_LIBRARY ${QT_QTCORE_LIBRARY} )
	    endif()
	endif()
    endif()

    if ( NOT DEFINED ZLIB_INCLUDE_DIR )
	 find_package( Zlib REQUIRED )
    endif()

    if ( NOT "${ZLIB_INCLUDE_DIR}" STREQUAL "" )
	list( APPEND OD_MODULE_INCLUDESYSPATH ${ZLIB_INCLUDE_DIR} )
	list( APPEND OD_MODULE_EXTERNAL_LIBS ${ZLIB_LIBRARY} )
	add_definitions( -DHAS_ZLIB )
    endif()
ENDMACRO()
