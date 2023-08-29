#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#

macro( OD_SETUP_SQLITE_TARGET )
    add_library( SQLite::SQLite3 SHARED IMPORTED GLOBAL )
    set( SQLite3_FOUND TRUE )
    if ( WIN32 )
	set( SQLITE_LIB_LOCATION_RELEASE "${LIBSQLITE}" )
	od_get_dll( "${LIBSQLITE}" SQLITE_LOCATION_RELEASE )
    else()
	get_filename_component( SQLITE_SONAME_RELEASE "${LIBSQLITE}" NAME )
	get_filename_component( SQLITE_LOCATION_RELEASE "${LIBSQLITE}" REALPATH )
    endif()
    set_target_properties( SQLite::SQLite3 PROPERTIES
	IMPORTED_CONFIGURATIONS RELEASE
	IMPORTED_LOCATION_RELEASE "${SQLITE_LOCATION_RELEASE}" )
    unset( SQLITE_LOCATION_RELEASE )
    if ( WIN32 )
	set_target_properties( SQLite::SQLite3 PROPERTIES
		IMPORTED_IMPLIB_RELEASE "${SQLITE_LIB_LOCATION_RELEASE}" )
	unset( SQLITE_LIB_LOCATION_RELEASE )
    else()
	set_target_properties( SQLite::SQLite3 PROPERTIES
		IMPORTED_SONAME_RELEASE "${SQLITE_SONAME_RELEASE}" )
	unset( SQLITE_SONAME_RELEASE )
    endif()
    unset( LIBSQLITE CACHE )
    od_map_configurations( SQLite::SQLite3 )
endmacro(OD_SETUP_SQLITE_TARGET)

macro( OD_GET_SQLITE_VERSION )
    if ( NOT DEFINED CACHE{SQLite3_VERSION} )
	find_program( SQLITE3_EXECUTABLE "sqlite3"
		      PATHS "${SQLITE_DIR}/bin" )
	if ( SQLITE3_EXECUTABLE AND EXISTS "${SQLITE3_EXECUTABLE}" )
	    execute_process( COMMAND "${SQLITE3_EXECUTABLE}" "--version"
			     OUTPUT_VARIABLE SQLITE3_VERSION_OUTPUT )
	    string( REPLACE " " ";" SQLITE3_VERSION_OUTPUT_FIELDS "${SQLITE3_VERSION_OUTPUT}" )
	    list( GET SQLITE3_VERSION_OUTPUT_FIELDS 0 SQLite3_VERSION )
	    set( SQLite3_VERSION ${SQLite3_VERSION} CACHE INTERNAL
		 "The version of sqlite3 which was detected" )
	endif()
    endif()
endmacro(OD_GET_SQLITE_VERSION )

macro( OD_FIND_SQLITE )

    if ( NOT SQLite3_FOUND AND NOT TARGET SQLite::SQLite3 )
	find_package( SQLite3 QUIET GLOBAL )
	if ( SQLite3_FOUND )
	    set( SQLite3_VERSION ${SQLite3_VERSION} CACHE INTERNAL
		 "The version of sqlite3 which was detected" )
	else()
	    unset( SQLite3_INCLUDE_DIR CACHE )
	    unset( SQLite3_LIBRARY CACHE )
	    #Setting the target from the library only
	    if ( DEFINED SQLITE_DIR )
		if ( UNIX )
		    SET( LIBSEARCHPATHS "${SQLITE_DIR}/lib64;${SQLITE_DIR}/lib" )
		else()
		    SET( LIBSEARCHPATHS "${SQLITE_DIR}/lib" )
		endif()
	    endif()

	    if ( WIN32 )
		od_find_library( LIBSQLITE sqlite3.lib )
	    elseif ( APPLE )
		SET( LIBSEARCHPATHS /usr/lib/sqlite3 )
		od_find_library( LIBSQLITE libtclsqlite3.dylib )
	    else()
		od_find_library( LIBSQLITE libsqlite3.so.0 )
	    endif()

	    if ( EXISTS "${LIBSQLITE}" )
		OD_SETUP_SQLITE_TARGET()
		OD_GET_SQLITE_VERSION()
	    endif()
	endif()
    endif()

endmacro(OD_FIND_SQLITE)

macro ( OD_SETUP_SQLITE )

    if ( SQLite3_FOUND AND TARGET SQLite::SQLite3 )
	if ( OD_LINKSQLITE )
	    list( APPEND OD_MODULE_EXTERNAL_LIBS SQLite::SQLite3 )
	elseif ( OD_USESQLITE )
	    list( APPEND OD_MODULE_EXTERNAL_RUNTIME_LIBS SQLite::SQLite3 )
	endif()
    else()
	set( SQLITE_DIR "" CACHE PATH "SQLite3 location" )
	message( SEND_ERROR "Cannot find/use the SQLite3 installation" )
    endif()

endmacro(OD_SETUP_SQLITE)
