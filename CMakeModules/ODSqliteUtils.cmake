#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#

macro( OD_FIND_SQLITE )

    if ( NOT TARGET SQLite::SQLite3 )
	find_package( SQLite3 QUIET CONFIG GLOBAL PATHS "${SQLite3_ROOT}" HINTS "${CMAKE_PREFIX_PATH}" NO_DEFAULT_PATH )
	if ( NOT TARGET SQLite::SQLite3 )
	    find_package( SQLite3 QUIET GLOBAL )
	    unset( SQLite3_DIR CACHE )
	endif()
	if ( TARGET SQLite::SQLite3 )
	    od_setup_external_target( SQLite::SQLite3 )
	    set( SQLite3_VERSION ${SQLite3_VERSION} CACHE INTERNAL
		"The version of SQLite3 which was detected" )
	    unset( SQLite3_ROOT CACHE )
	endif()
    endif()

endmacro(OD_FIND_SQLITE)

macro( OD_SETUP_SQLITE )

    if ( SQLite3_FOUND AND TARGET SQLite::SQLite3 )
	if ( OD_LINKSQLITE )
	    list( APPEND OD_MODULE_EXTERNAL_LIBS SQLite::SQLite3 )
	elseif ( OD_USESQLITE )
	    list( APPEND OD_MODULE_EXTERNAL_RUNTIME_LIBS SQLite::SQLite3 )
	endif()
    else()
	set( SQLite3_ROOT "" CACHE PATH "SQLite3 location" )
	message( SEND_ERROR "Cannot find/use the SQLite3 installation" )
    endif()

endmacro(OD_SETUP_SQLITE)
