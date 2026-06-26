#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#

# ZLIB
macro( OD_FIND_ZLIB )

    if ( NOT ZLIB_FOUND )

	if ( NOT OD_NO_QT )
	    OD_FIND_QT()
	endif()
	if ( QT_VERSION_MAJOR AND NOT OD_NO_QT )
	    OD_ADD_QT_COMPS( Core QUIET )
	    if ( Qt${QT_VERSION_MAJOR}Core_FOUND )
		if ( QT_VERSION_MAJOR GREATER_EQUAL 6 )
		    OD_ADD_QT_COMPS( ZlibPrivate QUIET )
		    if ( Qt${QT_VERSION_MAJOR}ZlibPrivate_FOUND )
			set ( ZLIB_LIBRARY Qt${QT_VERSION_MAJOR}::Core )
			set ( ZLIB_FOUND TRUE )
		    endif()
		elseif ( QT_VERSION_MAJOR EQUAL 5 )
		    OD_ADD_QT_COMPS( Zlib )
		    if ( Qt${QT_VERSION_MAJOR}Zlib_FOUND )
			set ( ZLIB_LIBRARY Qt${QT_VERSION_MAJOR}::Core )
			set ( ZLIB_FOUND TRUE )
		    endif()
		endif()
	    endif()
	endif()

	if ( NOT ZLIB_FOUND )
	    find_package( ZLIB QUIET GLOBAL CONFIG PATHS "${ZLIB_ROOT}" HINTS "${CMAKE_PREFIX_PATH}" NO_DEFAULT_PATH )
	    if ( NOT TARGET ZLIB::ZLIB )
		find_package( ZLIB QUIET GLOBAL )
		unset( ZLIB_DIR CACHE )
	    endif()
	    if ( TARGET ZLIB::ZLIB )
		od_setup_external_target( ZLIB::ZLIB )
		if ( IS_DIRECTORY "$CACHE{ZLIB_ROOT}" )
		    set( ZLIB_ROOT "$CACHE{ZLIB_ROOT}" )
		endif()
		unset( ZLIB_ROOT CACHE )
	    endif()
	endif()

    endif()

endmacro(OD_FIND_ZLIB)

macro( OD_SETUP_ZLIB )

    if ( ZLIB_FOUND )
	if ( TARGET ${ZLIB_LIBRARY} AND ${ZLIB_LIBRARY} STREQUAL "Qt${QT_VERSION_MAJOR}::Core" )
	    if ( QT_VERSION_MAJOR GREATER_EQUAL 6 AND Qt${QT_VERSION_MAJOR}ZlibPrivate_FOUND )
		list( APPEND OD_MODULE_EXTERNAL_LIBS "Qt${QT_VERSION_MAJOR}::ZlibPrivate" )
	    elseif ( QT_VERSION_MAJOR EQUAL 5 AND Qt${QT_VERSION_MAJOR}Zlib_FOUND )
		list( APPEND OD_MODULE_EXTERNAL_LIBS "Qt${QT_VERSION_MAJOR}::Zlib" )
	    endif()
	elseif ( TARGET ZLIB::ZLIB )
	    list( APPEND OD_MODULE_EXTERNAL_LIBS ZLIB::ZLIB )
	endif()

	list( APPEND OD_MODULE_COMPILE_DEFINITIONS "HAS_ZLIB" )
    else()
	set( ZLIB_ROOT "" CACHE PATH "ZLIB location" )
	message( SEND_ERROR "Cannot find/use the ZLIB installation" )
    endif()

endmacro(OD_SETUP_ZLIB)


# PNG::PNG
macro( OD_FIND_PNG )

    if ( NOT TARGET ZLIB::ZLIB )
	OD_FIND_ZLIB()
    endif()

    if ( NOT TARGET PNG::PNG AND TARGET ZLIB::ZLIB )
	find_package( PNG QUIET CONFIG GLOBAL PATHS "${PNG_ROOT}" HINTS "${CMAKE_PREFIX_PATH}" NO_DEFAULT_PATH )
	if ( NOT TARGET PNG::PNG )
	    find_package( PNG QUIET GLOBAL )
	    unset( PNG_DIR CACHE )
	endif()
	if ( TARGET PNG::PNG )
	    od_setup_external_target( PNG::PNG )
	    if ( IS_DIRECTORY "$CACHE{PNG_ROOT}" )
		set( PNG_ROOT "$CACHE{PNG_ROOT}" )
	    endif()
	    unset( PNG_ROOT CACHE )
	else()
	    set( PNG_ROOT "" CACHE PATH "PNG location" )
	    message( SEND_ERROR "Cannot find/use the PNG library" )
	endif()
    endif()

endmacro(OD_FIND_PNG)


# Freetype::Freetype
macro( OD_FIND_Freetype )

    if ( UNIX AND NOT TARGET PNG::PNG )
	OD_FIND_PNG()
    endif()

    if ( NOT TARGET Freetype::Freetype )
	find_package( Freetype QUIET CONFIG GLOBAL PATHS "${Freetype_ROOT}" HINTS "${CMAKE_PREFIX_PATH}" NO_DEFAULT_PATH )
	if ( NOT TARGET Freetype::Freetype )
	    find_package( Freetype QUIET GLOBAL )
	    unset( Freetype_DIR CACHE )
	endif()
	if ( TARGET Freetype::Freetype )
	    od_setup_external_target( Freetype::Freetype )
	    if ( IS_DIRECTORY "$CACHE{Freetype_ROOT}" )
		set( Freetype_ROOT "$CACHE{Freetype_ROOT}" )
	    endif()
	    unset( Freetype_ROOT CACHE )
	else()
	    set( Freetype_ROOT "" CACHE PATH "Freetype location" )
	    message( SEND_ERROR "Cannot find/use the Freetype library" )
	endif()
    endif()

endmacro(OD_FIND_Freetype)


# Fontconfig::Fontconfig
macro( OD_FIND_Fontconfig )

    if ( NOT TARGET Freetype::Freetype )
	OD_FIND_Freetype()
    endif()

    if ( NOT TARGET Fontconfig::Fontconfig AND TARGET Freetype::Freetype )
	find_package( Fontconfig QUIET CONFIG GLOBAL PATHS "${Fontconfig_ROOT}" HINTS "${CMAKE_PREFIX_PATH}" NO_DEFAULT_PATH )
	if ( NOT TARGET Fontconfig::Fontconfig )
	    find_package( Fontconfig QUIET GLOBAL )
	    unset( Fontconfig_DIR CACHE )
	endif()
	if ( TARGET Fontconfig::Fontconfig )
	    od_setup_external_target( Fontconfig::Fontconfig )
	    if ( IS_DIRECTORY "$CACHE{Fontconfig_ROOT}" )
		set( Fontconfig_ROOT "$CACHE{Fontconfig_ROOT}" )
	    endif()
	    unset( Fontconfig_ROOT CACHE )
	else()
	    set( Fontconfig_ROOT "" CACHE PATH "Fontconfig location" )
	    message( SEND_ERROR "Cannot find/use the Fontconfig library" )
	endif()
    endif()

endmacro(OD_FIND_Fontconfig)


# Cups::Cups
macro( OD_SETUP_Cups_TARGET )
    get_filename_component( CUPS_LOCATION "${CUPS_LIBRARIES}" REALPATH )
    get_filename_component( CUPS_SONAME "${CUPS_LOCATION}" NAME )
    set_target_properties( Cups::Cups PROPERTIES
	IMPORTED_CONFIGURATIONS "RELEASE"
	IMPORTED_LOCATION_RELEASE "${CUPS_LOCATION}"
	IMPORTED_SONAME_RELEASE "\@rpath/${CUPS_SONAME}" )
    unset( CUPS_SONAME )
    unset( CUPS_LOCATION )
endmacro(OD_SETUP_Cups_TARGET)

macro( OD_FIND_Cups )

    if ( NOT TARGET Cups::Cups )
	find_package( Cups QUIET CONFIG GLOBAL PATHS "${Cups_ROOT}" HINTS "${CMAKE_PREFIX_PATH}" NO_DEFAULT_PATH )
	if ( NOT TARGET Cups::Cups )
	    find_package( Cups QUIET GLOBAL )
	    unset( Cups_DIR CACHE )
	endif()
	if ( TARGET Cups::Cups )
	    OD_SETUP_Cups_TARGET()
	    od_setup_external_target( Cups::Cups )
	    unset( Cups_ROOT CACHE )
	else()
	    set( Cups_ROOT "" CACHE PATH "Cups location" )
	    message( SEND_ERROR "Cannot find/use the Cups library" )
	endif()
    endif()

endmacro(OD_FIND_Cups)
