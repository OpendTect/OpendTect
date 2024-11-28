#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#

# ZLIB
macro( OD_SETUP_ZLIB_TARGET )
    get_target_property( ZLIB_LOCATION ZLIB::ZLIB IMPORTED_LOCATION_RELEASE )
    if ( ZLIB_LOCATION AND EXISTS "${ZLIB_LOCATION}" )
	get_filename_component( ZLIB_IMPORTED_LOCATION_RELEASE "${ZLIB_LOCATION}" REALPATH )
	set_target_properties( ZLIB::ZLIB PROPERTIES
			       IMPORTED_LOCATION_RELEASE "${ZLIB_IMPORTED_LOCATION_RELEASE}" )
	if ( UNIX )
	    get_filename_component( ZLIB_SONAME "${ZLIB_IMPORTED_LOCATION_RELEASE}" NAME_WLE )
	    get_filename_component( ZLIB_SONAME ${ZLIB_SONAME} NAME_WLE )
	    if ( APPLE )
		get_filename_component( ZLIB_SONAME ${ZLIB_SONAME} NAME_WLE )
		set( ZLIB_SONAME "@rpath/${ZLIB_SONAME}.dylib" )
	    endif()
	    set_target_properties( ZLIB::ZLIB PROPERTIES
		IMPORTED_SONAME_RELEASE ${ZLIB_SONAME} )
	    unset( IMPORTED_SONAME_RELEASE )
	endif()
	unset( ZLIB_IMPORTED_LOCATION_RELEASE )
	od_map_configurations( ZLIB::ZLIB )
    endif()
    unset( ZLIB_LOCATION )
endmacro(OD_SETUP_ZLIB_TARGET)


macro( OD_FIND_ZLIB )

    if ( NOT ZLIB_FOUND )

	if ( NOT OD_NO_QT )
	    OD_FIND_QT()
	    find_package( QT NAMES Qt6 Qt5 QUIET COMPONENTS Core GLOBAL )
	endif()
	if ( QT_VERSION_MAJOR AND NOT OD_NO_QT )
	    find_package( Qt${QT_VERSION_MAJOR} QUIET COMPONENTS Core )
	    if ( Qt${QT_VERSION_MAJOR}Core_FOUND )
		if ( QT_VERSION_MAJOR GREATER_EQUAL 6 )
		    find_package( Qt${QT_VERSION_MAJOR} QUIET COMPONENTS ZlibPrivate GLOBAL )
		    if ( Qt${QT_VERSION_MAJOR}ZlibPrivate_FOUND )
			set ( ZLIB_LIBRARY Qt${QT_VERSION_MAJOR}::Core )
			set ( ZLIB_FOUND TRUE )
		    endif()
		elseif ( QT_VERSION_MAJOR EQUAL 5 )
		    find_package( Qt${QT_VERSION_MAJOR} QUIET COMPONENTS Zlib GLOBAL )
		    if ( Qt${QT_VERSION_MAJOR}Zlib_FOUND )
			set ( ZLIB_LIBRARY Qt${QT_VERSION_MAJOR}::Core )
			set ( ZLIB_FOUND TRUE )
		    endif()
		endif()
	    endif()
	endif()

	if ( NOT ZLIB_FOUND )
	    if ( NOT DEFINED ZLIB_ROOT AND IS_DIRECTORY "${ZLIB_INCLUDE_DIR}" )
		get_filename_component( ZLIB_ROOT "${ZLIB_INCLUDE_DIR}" DIRECTORY )
	    endif()
	    find_package( ZLIB QUIET GLOBAL )
	    if ( ZLIB_FOUND )
		OD_SETUP_ZLIB_TARGET()
		unset( ZLIB_ROOT CACHE )
	    endif()
	endif()

	if ( NOT ZLIB_FOUND )
	    unset( ZLIB_INCLUDE_DIR CACHE )
	    unset( ZLIB_LIBRARY_DEBUG CACHE )
	    unset( ZLIB_LIBRARY_RELEASE CACHE )
	endif()
    endif()

endmacro(OD_FIND_ZLIB)


macro( OD_SETUP_ZLIB )

    if ( ZLIB_FOUND )
	if ( TARGET ZLIB::ZLIB )
	    list( APPEND OD_MODULE_EXTERNAL_LIBS ZLIB::ZLIB )
	elseif ( TARGET ${ZLIB_LIBRARY} AND ${ZLIB_LIBRARY} STREQUAL "Qt${QT_VERSION_MAJOR}::Core" )
	    if ( QT_VERSION_MAJOR GREATER_EQUAL 6 AND Qt${QT_VERSION_MAJOR}ZlibPrivate_FOUND )
		list( APPEND OD_MODULE_EXTERNAL_LIBS "Qt${QT_VERSION_MAJOR}::ZlibPrivate" )
	    elseif ( QT_VERSION_MAJOR EQUAL 5 AND Qt${QT_VERSION_MAJOR}Zlib_FOUND )
		list( APPEND OD_MODULE_EXTERNAL_LIBS "Qt${QT_VERSION_MAJOR}::Zlib" )
	    endif()
	endif()

	list( APPEND OD_MODULE_COMPILE_DEFINITIONS "HAS_ZLIB" )
    else()
	set( ZLIB_ROOT "" CACHE PATH "ZLIB location" )
	message( SEND_ERROR "Cannot find/use the ZLIB installation" )
    endif()

endmacro(OD_SETUP_ZLIB)


# PNG::PNG
macro( OD_SETUP_PNG_TARGET )
    get_target_property( PNG_LOCATION PNG::PNG IMPORTED_LOCATION_RELEASE )
    if ( PNG_LOCATION AND EXISTS "${PNG_LOCATION}" )
	get_filename_component( PNG_IMPORTED_LOCATION_RELEASE "${PNG_LOCATION}" REALPATH )
	set_target_properties( PNG::PNG PROPERTIES
			       IMPORTED_LOCATION "${PNG_IMPORTED_LOCATION_RELEASE}"
			       IMPORTED_LOCATION_RELEASE "${PNG_IMPORTED_LOCATION_RELEASE}" )
	if ( UNIX )
	    get_filename_component( PNG_SONAME "${PNG_IMPORTED_LOCATION_RELEASE}" NAME_WLE )
	    get_filename_component( PNG_SONAME ${PNG_SONAME} NAME_WLE )
	    if ( APPLE )
		get_filename_component( PNG_SONAME ${PNG_SONAME} NAME_WLE )
		set( PNG_SONAME "@rpath/${PNG_SONAME}.dylib" )
	    endif()
	    set_target_properties( PNG::PNG PROPERTIES
		IMPORTED_SONAME_RELEASE ${PNG_SONAME} )
	    unset( IMPORTED_SONAME_RELEASE )
	endif()
	unset( PNG_IMPORTED_LOCATION_RELEASE )
	od_map_configurations( PNG::PNG )
    endif()
    unset( PNG_LOCATION )
endmacro(OD_SETUP_PNG_TARGET)


macro( OD_FIND_PNG )

    if ( NOT TARGET ZLIB::ZLIB )
	OD_FIND_ZLIB()
    endif()

    if ( NOT TARGET PNG::PNG AND TARGET ZLIB::ZLIB )
	if ( NOT DEFINED PNG_ROOT AND IS_DIRECTORY "${PNG_PNG_INCLUDE_DIR}" )
	    get_filename_component( PNG_ROOT "${PNG_PNG_INCLUDE_DIR}" DIRECTORY )
	endif()
	find_package( PNG QUIET GLOBAL )
	if ( TARGET PNG::PNG )
	    OD_SETUP_PNG_TARGET()
	    unset( PNG_ROOT CACHE )
	else()
	    set( PNG_ROOT "" CACHE PATH "PNG location" )
	    message( SEND_ERROR "Cannot find/use the PNG library" )
	endif()
    endif()

endmacro(OD_FIND_PNG)


# Freetype::Freetype
macro( OD_SETUP_Freetype_TARGET )
    get_target_property( Freetype_LOCATION Freetype::Freetype IMPORTED_LOCATION_RELEASE )
    if ( Freetype_LOCATION AND EXISTS "${Freetype_LOCATION}" )
	get_filename_component( Freetype_IMPORTED_LOCATION_RELEASE "${Freetype_LOCATION}" REALPATH )
	set_target_properties( Freetype::Freetype PROPERTIES
			       IMPORTED_LOCATION "${Freetype_IMPORTED_LOCATION_RELEASE}"
			       IMPORTED_LOCATION_RELEASE "${Freetype_IMPORTED_LOCATION_RELEASE}" )
	if ( UNIX )
	    get_filename_component( Freetype_SONAME "${Freetype_IMPORTED_LOCATION_RELEASE}" NAME_WLE )
	    get_filename_component( Freetype_SONAME ${Freetype_SONAME} NAME_WLE )
	    if ( APPLE )
		get_filename_component( Freetype_SONAME ${Freetype_SONAME} NAME_WLE )
		set( Freetype_SONAME "@rpath/${Freetype_SONAME}.dylib" )
	    endif()
	    set_target_properties( Freetype::Freetype PROPERTIES
		IMPORTED_SONAME_RELEASE ${Freetype_SONAME} )
	    unset( IMPORTED_SONAME_RELEASE )
	endif()
	unset( Freetype_IMPORTED_LOCATION_RELEASE )
	od_map_configurations( Freetype::Freetype )
    endif()
    unset( Freetype_LOCATION )
endmacro(OD_SETUP_Freetype_TARGET)


macro( OD_FIND_Freetype )

    if ( UNIX AND NOT APPLE AND NOT TARGET PNG::PNG )
	OD_FIND_PNG()
    endif()

    if ( NOT TARGET Freetype::Freetype )
	if ( NOT DEFINED Freetype_ROOT AND IS_DIRECTORY "${FREETYPE_INCLUDE_DIR_freetype2}" )
	    get_filename_component( Freetype_ROOT "${FREETYPE_INCLUDE_DIR_freetype2}" DIRECTORY )
	endif()
	find_package( Freetype QUIET GLOBAL )
	if ( TARGET Freetype::Freetype )
	    OD_SETUP_Freetype_TARGET()
	    unset( Freetype_ROOT CACHE )
	else()
	    set( Freetype_ROOT "" CACHE PATH "Freetype location" )
	    message( SEND_ERROR "Cannot find/use the Freetype library" )
	endif()
    endif()

endmacro(OD_FIND_Freetype)


# Fontconfig::Fontconfig
macro( OD_SETUP_Fontconfig_TARGET )
    get_target_property( Fontconfig_LOCATION Fontconfig::Fontconfig IMPORTED_LOCATION )
    if ( Fontconfig_LOCATION AND EXISTS "${Fontconfig_LOCATION}" )
	get_filename_component( Fontconfig_IMPORTED_LOCATION_RELEASE "${Fontconfig_LOCATION}" REALPATH )
	set_target_properties( Fontconfig::Fontconfig PROPERTIES
			       IMPORTED_CONFIGURATIONS RELEASE
			       IMPORTED_LOCATION "${Fontconfig_IMPORTED_LOCATION_RELEASE}"
			       IMPORTED_LOCATION_RELEASE "${Fontconfig_IMPORTED_LOCATION_RELEASE}" )
	if ( UNIX )
	    get_filename_component( Fontconfig_SONAME "${Fontconfig_IMPORTED_LOCATION_RELEASE}" NAME_WLE )
	    get_filename_component( Fontconfig_SONAME ${Fontconfig_SONAME} NAME_WLE )
	    if ( APPLE )
		get_filename_component( Fontconfig_SONAME ${Fontconfig_SONAME} NAME_WLE )
		set( Fontconfig_SONAME "@rpath/${Fontconfig_SONAME}.dylib" )
	    endif()
	    set_target_properties( Fontconfig::Fontconfig PROPERTIES
		IMPORTED_SONAME_RELEASE ${Fontconfig_SONAME} )
	    unset( IMPORTED_SONAME_RELEASE )
	endif()
	unset( Fontconfig_IMPORTED_LOCATION_RELEASE )
	od_map_configurations( Fontconfig::Fontconfig )
    endif()
    unset( Fontconfig_LOCATION )
endmacro(OD_SETUP_Fontconfig_TARGET)


macro( OD_FIND_Fontconfig )

    if ( NOT TARGET Freetype::Freetype )
	OD_FIND_Freetype()
    endif()

    if ( NOT TARGET Fontconfig::Fontconfig AND TARGET Freetype::Freetype )
	if ( NOT DEFINED Fontconfig_ROOT AND IS_DIRECTORY "${Fontconfig_INCLUDE_DIR}" )
	    get_filename_component( Fontconfig_ROOT "${Fontconfig_INCLUDE_DIR}" DIRECTORY )
	endif()
	find_package( Fontconfig QUIET GLOBAL )
	if ( TARGET Fontconfig::Fontconfig )
	    OD_SETUP_Fontconfig_TARGET()
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
	IMPORTED_SONAME_RELEASE "${CUPS_SONAME}" )
    unset( CUPS_SONAME )
    unset( CUPS_LOCATION )
    od_map_configurations( Cups::Cups )
endmacro(OD_SETUP_Cups_TARGET)


macro( OD_FIND_Cups )

    if ( NOT TARGET Cups::Cups )
	if ( NOT DEFINED Cups_ROOT AND IS_DIRECTORY "${CUPS_INCLUDE_DIR}" )
	    get_filename_component( Cups_ROOT "${CUPS_INCLUDE_DIR}" DIRECTORY )
	endif()
	find_package( Cups QUIET GLOBAL )
	if ( TARGET Cups::Cups )
	    OD_SETUP_Cups_TARGET()
	    unset( Cups_ROOT CACHE )
	else()
	    set( Cups_ROOT "" CACHE PATH "Cups location" )
	    message( SEND_ERROR "Cannot find/use the Cups library" )
	endif()
    endif()

endmacro(OD_FIND_Cups)
