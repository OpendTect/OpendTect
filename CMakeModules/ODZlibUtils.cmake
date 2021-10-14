#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#_______________________________________________________________________________

macro( OD_ADD_SYSZLIB )
    find_package( ZLIB QUIET )
endmacro()

macro( OD_ADD_ZLIB )

    if ( Qt5Core_FOUND )
	find_package( Qt5 QUIET COMPONENTS Zlib )
	if ( Qt5Zlib_FOUND )
	    set ( ZLIB_LIBRARY Qt5::Core )
	    set ( ZLIB_FOUND True )
	else()
	    OD_ADD_SYSZLIB()
	endif()
    else()
	OD_ADD_SYSZLIB()
    endif()

    if ( NOT ZLIB_FOUND )
	message( FATAL_ERROR "ZLib external not found" )
    endif()

endmacro(OD_ADD_ZLIB)

macro( OD_SETUP_ZLIB )

    if ( OD_USEZLIB )
        if( TARGET Qt5::Zlib )
	    get_target_property( ZLIB_INCLUDE_DIR Qt5::Zlib INTERFACE_INCLUDE_DIRECTORIES )
	endif()

	if ( ZLIB_INCLUDE_DIR )
	    list( APPEND OD_MODULE_INCLUDESYSPATH ${ZLIB_INCLUDE_DIR} )
	    list( APPEND OD_MODULE_EXTERNAL_LIBS ${ZLIB_LIBRARY_RELEASE} )
	    add_definitions( -DHAS_ZLIB )
	endif()
    endif()

endmacro()
