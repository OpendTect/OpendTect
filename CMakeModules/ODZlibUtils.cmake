#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#_______________________________________________________________________________

macro( OD_ADD_SYSZLIB )
    find_package( ZLIB QUIET )
endmacro()

macro( OD_ADD_ZLIB )

    if ( Qt${QT_VERSION_MAJOR}Core_FOUND )
	if ( QT_VERSION_MAJOR EQUAL 5 )
	    find_package( Qt${QT_VERSION_MAJOR} QUIET COMPONENTS Zlib )
	    if ( Qt${QT_VERSION_MAJOR}Zlib_FOUND )
		set ( ZLIB_LIBRARY Qt${QT_VERSION_MAJOR}::Core )
		set ( ZLIB_FOUND True )
	    endif()
	elseif( QT_VERSION_MAJOR GREATER_EQUAL 6 )
	    find_package( Qt${QT_VERSION_MAJOR} QUIET COMPONENTS ZlibPrivate )
	    if ( Qt${QT_VERSION_MAJOR}ZlibPrivate_FOUND )
		set ( ZLIB_LIBRARY Qt${QT_VERSION_MAJOR}::Core )
		set ( ZLIB_FOUND True )
	    endif()
	endif()
    endif()

    if ( NOT ZLIB_FOUND )
	OD_ADD_SYSZLIB()
    endif()

    if ( NOT ZLIB_FOUND )
	message( FATAL_ERROR "ZLib external not found" )
    endif()

endmacro(OD_ADD_ZLIB)

macro( OD_SETUP_ZLIB )

    if ( OD_USEZLIB )
	if ( QT_VERSION_MAJOR EQUAL 5 AND Qt${QT_VERSION_MAJOR}Zlib_FOUND )
	    get_target_property( ZLIB_INCLUDE_DIR Qt${QT_VERSION_MAJOR}::Zlib INTERFACE_INCLUDE_DIRECTORIES )
	elseif ( QT_VERSION_MAJOR GREATER_EQUAL 6 AND Qt${QT_VERSION_MAJOR}ZlibPrivate_FOUND )
	    get_target_property( ZLIB_INCLUDE_DIR Qt${QT_VERSION_MAJOR}::ZlibPrivate INTERFACE_INCLUDE_DIRECTORIES )
	endif()

	if ( ZLIB_INCLUDE_DIR )
	    list( APPEND OD_MODULE_INCLUDESYSPATH ${ZLIB_INCLUDE_DIR} )
	    if ( EXISTS "${ZLIB_LIBRARY_RELEASE}" )
		list( APPEND OD_MODULE_EXTERNAL_LIBS ${ZLIB_LIBRARY_RELEASE} )
	    elseif ( EXISTS "${ZLIB_LIBRARY_DEBUG}" )
		list( APPEND OD_MODULE_EXTERNAL_LIBS ${ZLIB_LIBRARY_DEBUG} )
	    elseif ( TARGET ${ZLIB_LIBRARY} )
		list( APPEND OD_MODULE_EXTERNAL_LIBS ${ZLIB_LIBRARY} )
	    endif()
	    add_definitions( -DHAS_ZLIB )
	endif()
    endif()

endmacro()
