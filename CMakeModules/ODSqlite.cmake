#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#

macro( OD_GET_SQLITE_VERSION )
    if ( NOT DEFINED CACHE{SQLite3_VERSION} OR "$CACHE{SQLite3_VERSION}" STREQUAL "" )
	find_program( SQLITE3_EXECUTABLE "sqlite3"
		      PATHS "${SQLITE_DIR}/bin" )
	if ( SQLITE3_EXECUTABLE AND EXISTS "${SQLITE3_EXECUTABLE}" )
	    execute_process( COMMAND "${SQLITE3_EXECUTABLE}" "--version"
			     OUTPUT_VARIABLE SQLITE3_VERSION_STDMSG
			     ERROR_VARIABLE SQLITE3_VERSION_ERRMSG
			     RESULT_VARIABLE SQLITE3_VERSION_STATUS )
	    if ( ${SQLITE3_VERSION_STATUS} EQUAL 0 )
		string( REPLACE " " ";" SQLITE3_VERSION_OUTPUT_FIELDS "${SQLITE3_VERSION_STDMSG}" )
		list( GET SQLITE3_VERSION_OUTPUT_FIELDS 0 SQLite3_VERSION )
		set( SQLite3_VERSION ${SQLite3_VERSION} CACHE INTERNAL
		     "The version of sqlite3 which was detected" )
	    endif()
	endif()
    endif()
endmacro(OD_GET_SQLITE_VERSION )

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

    if ( EXISTS "${LIBSQLITE}" )
	OD_GET_SQLITE_VERSION()
    else()
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
