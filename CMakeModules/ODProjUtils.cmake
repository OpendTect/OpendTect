#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#

macro( OD_FIND_PROJ )

    if ( NOT OD_NO_PROJ AND NOT TARGET PROJ::proj )
	if ( NOT EXISTS "${PROJ_DIR}/proj-config.cmake" )
	    set ( PROJ_DIR "${PROJ_DIR}/lib/cmake/proj" )
	endif()

	find_package( PROJ QUIET GLOBAL )
	if ( TARGET PROJ::proj )
	    od_map_configurations( PROJ::proj )
	endif()
    endif()

endmacro(OD_FIND_PROJ)

macro( OD_SETUP_PROJ )

    if ( OD_NO_PROJ )
	list( APPEND OD_MODULE_COMPILE_DEFINITIONS "OD_NO_PROJ" )
    else()
	if ( PROJ_FOUND AND TARGET ${PROJ_LIBRARIES} )
	    if ( OD_LINKPROJ )
		list( APPEND OD_MODULE_EXTERNAL_LIBS "${PROJ_LIBRARIES}" )
	    elseif ( OD_USEPROJ )
		list( APPEND OD_MODULE_EXTERNAL_RUNTIME_LIBS "${PROJ_LIBRARIES}" )
	    endif()
	else()
	    set( PROJ_DIR "" CACHE PATH "PROJ location" )
	    message( SEND_ERROR "Cannot find/use the PROJ installation" )
	endif()

	install( FILES "${PROJ_INCLUDE_DIRS}/../share/proj/proj.db"
		 DESTINATION "${OD_DATA_INSTALL_RELPATH}/CRS" )
	install( FILES "${PROJ_INCLUDE_DIRS}/../share/doc/proj/COPYING"
		 DESTINATION "${OD_DATA_INSTALL_RELPATH}/CRS" )
    endif()

endmacro(OD_SETUP_PROJ)
