#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODInitheader.cmake,v 1.4 2012-08-03 11:31:30 cvskris Exp $
#_______________________________________________________________________________

# OD_CREATE_INIT_HEADER
# 
# OD_MODULE_NAME			: Name of the module, or the plugin
#

MACRO( OD_CREATE_INIT_HEADER )
    
    STRING ( TOUPPER ${OD_MODULE_NAME} OD_MODULE_NAME_UPPER )
    STRING ( TOLOWER ${OD_MODULE_NAME} OD_MODULE_NAME_LOWER )

    IF ( OD_IS_PLUGIN )
	SET( INCLUDEDIR ${CMAKE_SOURCE_DIR}/plugins/${OD_MODULE_NAME} )
    ELSE()
	if ( EXISTS ${CMAKE_SOURCE_DIR}/include/${OD_MODULE_NAME} )
	    SET( INCLUDEDIR ${CMAKE_SOURCE_DIR}/include/${OD_MODULE_NAME} )
	else()
	    if ( EXISTS ${CMAKE_SOURCE_DIR}/spec/${OD_MODULE_NAME} )
		SET( INCLUDEDIR ${CMAKE_SOURCE_DIR}/spec/${OD_MODULE_NAME} )
	    endif()
	endif()
    ENDIF()

    IF ( EXISTS ${INCLUDEDIR} )
	SET( INITHEADER ${INCLUDEDIR}/${OD_MODULE_NAME_LOWER}mod.h )

	CONFIGURE_FILE( ${OpendTect_DIR}/CMakeModules/templates/initheader.h.in 
			${INITHEADER} )
    ENDIF()
ENDMACRO()
