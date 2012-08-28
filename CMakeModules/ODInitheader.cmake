#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODInitheader.cmake,v 1.5 2012-08-28 12:12:53 cvskris Exp $
#_______________________________________________________________________________

# OD_CREATE_INIT_HEADER
# 
# OD_MODULE_NAME			: Name of the module, or the plugin
#

MACRO( OD_CREATE_INIT_HEADER )
    
    STRING ( TOUPPER ${OD_MODULE_NAME} OD_MODULE_NAME_UPPER )
    STRING ( TOLOWER ${OD_MODULE_NAME} OD_MODULE_NAME_LOWER )

    if ( OD_IS_PLUGIN )
	set ( INCLUDEDIR ${CMAKE_SOURCE_DIR}/plugins/${OD_MODULE_NAME} )
    else ()
	if ( EXISTS ${CMAKE_SOURCE_DIR}/include/${OD_MODULE_NAME} )
	    set ( INCLUDEDIR ${CMAKE_SOURCE_DIR}/include/${OD_MODULE_NAME} )
	else()
	    if ( EXISTS ${CMAKE_SOURCE_DIR}/spec/${OD_MODULE_NAME} )
		set ( INCLUDEDIR ${CMAKE_SOURCE_DIR}/spec/${OD_MODULE_NAME} )
	    endif()
	endif()
    endif ()

    if ( EXISTS ${INCLUDEDIR} )
	set ( INITHEADER ${INCLUDEDIR}/${OD_MODULE_NAME_LOWER}mod.h )
	set ( EXPORTHEADER ${OD_MODULE_NAME_LOWER}export.h )
	if ( EXISTS ${INCLUDEDIR}/${EXPORTHEADER} )
	    set ( MODFILEHEADER "#include \"${EXPORTHEADER}\"" )
	endif()

	CONFIGURE_FILE( ${OpendTect_DIR}/CMakeModules/templates/initheader.h.in 
			${INITHEADER} )
    endif ()
ENDMACRO()
