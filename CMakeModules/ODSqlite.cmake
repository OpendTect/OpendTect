#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#

macro( OD_GET_SQLITE )
   if ( DEFINED SQLITE_DIR )
	if ( UNIX )
	    SET( LIBSEARCHPATHS "${SQLITE_DIR}/lib64;${SQLITE_DIR}/lib" )
	else()
	    SET( LIBSEARCHPATHS "${SQLITE_DIR}/lib" )
	endif()
    endif()

    if ( APPLE )
	SET( LIBSEARCHPATHS /usr/lib/sqlite3 )
	od_find_library( LIBSQLITE libtclsqlite3.dylib )
    elseif ( WIN32 )
	od_find_library( LIBSQLITE sqlite3.lib )
    else()
	od_find_library( LIBSQLITE libsqlite3.so.0 )
    endif()

    if ( NOT EXISTS "${LIBSQLITE}" )
	set( SQLITE_DIR "" CACHE PATH "SQLite3 location" )
	message( FATAL_ERROR "SQLITE_DIR is not defined" )
    endif()
endmacro()

macro ( OD_SETUP_SQLITE )
    if ( OD_USESQLITE OR OD_LINKSQLITE )
	OD_GET_SQLITE()
    endif()

    if ( EXISTS "${LIBSQLITE}" )
	if ( OD_LINKSQLITE )
	    get_filename_component( LIBPATH "${LIBSQLITE}" DIRECTORY )
	    get_filename_component( LIBPATH "${LIBPATH}" DIRECTORY )
	    list( APPEND OD_MODULE_INCLUDESYSPATH  "${LIBPATH}/include" )
	    list( APPEND OD_MODULE_EXTERNAL_LIBS "${LIBSQLITE}" )
	elseif ( OD_USESQLITE )
	    list( APPEND OD_MODULE_EXTERNAL_RUNTIME_LIBS "${LIBSQLITE}" )
	endif()
    endif()
endmacro( OD_SETUP_SQLITE )
