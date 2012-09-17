#_______________________Pmake___________________________________________________
#
#	Makefile : 	Tutorial plugin
# 	Jan 2012	A.H. Bril
#	RCS :		$Id: CMakeLists.txt,v 1.4 2012/04/17 11:29:00 cvsraman Exp $
#_______________________________________________________________________________


cmake_minimum_required (VERSION 2.6)
PROJECT(OpendTect_Tutorial_Plugin)

SET ( OD_SUBSYSTEM "tut_plugin" )
SET ( OpendTect_DIR "" CACHE PATH "OpendTect directory" )
SET ( OD_BINARY_BASEDIR ${CMAKE_BINARY_DIR} )

LIST(APPEND CMAKE_MODULE_PATH "${OpendTect_DIR}/CMakeModules")
FIND_PACKAGE( OpendTect )

#Setup the modules
SET ( PLUGINS Tut uiTut My_Plugin_Name )
OD_ADD_MODULES( plugins ${PLUGINS} )

IF( OD_CREATE_LAUNCHERS )
    add_custom_target( ${OD_MAIN_EXEC} ALL )
    foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
	set_target_properties( ${OD_MAIN_EXEC}
	PROPERTIES
	LOCATION_${CONFIG}
	${OpendTect_DIR}/${OD_EXEC_OUTPUT_RELPATH}/${CONFIG}/${OD_MAIN_EXEC}${OD_EXECUTABLE_EXTENSION} )
    endforeach()

    create_target_launcher( ${OD_MAIN_EXEC}
    	RUNTIME_LIBRARY_DIRS
	 ${OpendTect_DIR}/${OD_EXEC_OUTPUT_RELPATH}
	 ${OD_BINARY_BASEDIR}/${OD_EXEC_OUTPUT_RELPATH} 
    	ENVIRONMENT
	DTECT_APPL=${OpendTect_DIR}
	WORK=${OpendTect_DIR} )
ENDIF( OD_CREATE_LAUNCHERS )

OD_WRITE_ALOFILES( ${CMAKE_CURRENT_BINARY_DIR} )
