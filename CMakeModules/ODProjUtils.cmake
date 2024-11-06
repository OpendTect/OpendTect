#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#

macro( OD_FIND_PROJ )

    if ( NOT OD_NO_PROJ AND NOT TARGET PROJ::proj )
	find_package( PROJ QUIET GLOBAL )
	if ( TARGET PROJ::proj )
	    od_map_configurations( PROJ::proj )
	    set_target_properties( PROJ::proj PROPERTIES
		IMPORTED_LINK_DEPENDENT_LIBRARIES SQLite::SQLite3 )
	    unset( PROJ_ROOT CACHE )
	endif()
    endif()

endmacro(OD_FIND_PROJ)

macro( OD_SETUP_PROJ )

    if ( NOT OD_NO_PROJ )
	if ( PROJ_FOUND AND TARGET ${PROJ_LIBRARIES} )
	    if ( OD_LINKPROJ )
		list( APPEND OD_MODULE_EXTERNAL_LIBS "${PROJ_LIBRARIES}" )
	    elseif ( OD_USEPROJ )
		list( APPEND OD_MODULE_EXTERNAL_RUNTIME_LIBS "${PROJ_LIBRARIES}" )
	    endif()
	else()
	    set( PROJ_ROOT "" CACHE PATH "PROJ location" )
	    message( SEND_ERROR "Cannot find/use the PROJ installation" )
	endif()

	get_filename_component( PROJ_DB_FILEPATH "${PROJ_INCLUDE_DIRS}/../share/proj/proj.db" ABSOLUTE )
	list( APPEND OD_MODULE_COMPILE_DEFINITIONS
		"__PROJ_DB_FILEPATH__=\"${PROJ_DB_FILEPATH}\"" )

	install( FILES "${PROJ_DB_FILEPATH}"
		 DESTINATION "${OD_DATA_INSTALL_RELPATH}/CRS" )
	install( FILES "${PROJ_INCLUDE_DIRS}/../share/doc/proj/COPYING"
		 DESTINATION "${OD_DATA_INSTALL_RELPATH}/CRS" )
    endif()

endmacro(OD_SETUP_PROJ)
