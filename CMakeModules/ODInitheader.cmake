#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODInitheader.cmake,v 1.3 2012-02-27 07:35:30 cvskris Exp $
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
	SET( INCLUDEDIR ${CMAKE_SOURCE_DIR}/include/${OD_MODULE_NAME} )
    ENDIF()

    IF ( EXISTS ${INCLUDEDIR} )
	SET( INITHEADER ${INCLUDEDIR}/init${OD_MODULE_NAME_LOWER}.h )

	CONFIGURE_FILE( ${OpendTect_DIR}/CMakeModules/templates/initheader.h.in 
			${INITHEADER} )
    ENDIF()
ENDMACRO()
