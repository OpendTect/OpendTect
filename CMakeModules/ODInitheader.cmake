#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODInitheader.cmake,v 1.2 2012-02-24 14:54:45 cvskris Exp $
#_______________________________________________________________________________

# OD_CREATE_INIT_HEADER
# 
# OD_MODULE_NAME			: Name of the module, or the plugin
#

MACRO( OD_CREATE_INIT_HEADER )
    
    STRING ( TOUPPER ${OD_MODULE_NAME} OD_MODULE_NAME_UPPER )
    STRING ( TOLOWER ${OD_MODULE_NAME} OD_MODULE_NAME_LOWER )

    IF ( OD_IS_PLUGIN )
	SET( INITHEADER ${CMAKE_SOURCE_DIR}/plugins/${OD_MODULE_NAME}/init${OD_MODULE_NAME_LOWER}.h )
    ELSE()
	SET( INITHEADER ${CMAKE_SOURCE_DIR}/include/${OD_MODULE_NAME}/init${OD_MODULE_NAME_LOWER}.h )
    ENDIF()

    CONFIGURE_FILE( ${OpendTect_DIR}/CMakeModules/templates/initheader.h.in 
		    ${INITHEADER} )
ENDMACRO()
