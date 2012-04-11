#_______________________Pmake___________________________________________________
#
#	Makefile : 	Tutorial plugin
# 	Jan 2012	A.H. Bril
#	RCS :		$Id: CMakeLists.txt,v 1.3 2012-04-11 11:02:29 cvskris Exp $
#_______________________________________________________________________________


cmake_minimum_required (VERSION 2.6)
PROJECT(OpendTect_Tutorial_Plugin)

SET ( OD_SUBSYSTEM "tut_plugin" )
SET ( OpendTect_DIR "" CACHE PATH "OpendTect directory" )

LIST(APPEND CMAKE_MODULE_PATH "${OpendTect_DIR}/CMakeModules")
FIND_PACKAGE( OpendTect )

OPTION ( OD_BUILD_LOCAL "Build in local directory" ON )

#Setup the modules
SET ( PLUGINS Tut uiTut )
OD_ADD_MODULES( plugins ${PLUGINS} )

IF( OD_CREATE_LAUNCHERS )
    add_custom_target( od_main ALL )
    foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
	set_target_properties( od_main
		PROPERTIES
		LOCATION_${CONFIG}
		${OD_EXEC_OUTPUT_PATH}/${CONFIG}/od_main )
    endforeach()

    create_target_launcher( od_main
    	RUNTIME_LIBRARY_DIRS
    	${OD_MODULE_RUNTIMEPATH}
    	${OD_LIB_OUTPUT_PATH}
    	${OD_PLUGIN_OUTPUT_PATH}
    	ENVIRONMENT
	DTECT_APPL=${OpendTect_DIR}
	WORK=${OpendTect_DIR} )
ENDIF( OD_CREATE_LAUNCHERS )

OD_WRITE_ALOFILES( ${CMAKE_CURRENT_BINARY_DIR} )
