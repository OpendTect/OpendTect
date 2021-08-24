#_______________________Pmake___________________________________________________
#
#	Makefile : 	Tutorial plugin
# 	Jan 2012	A.H. Bril
#_______________________________________________________________________________


cmake_minimum_required (VERSION 2.8.12)
project(OpendTect_Tutorial_Plugin)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set ( OD_SUBSYSTEM "tut_plugin" )
set ( OpendTect_DIR "" CACHE PATH "OpendTect directory" )


#Setup output directory
option ( OD_BUILD_LOCAL "Build in local directory" ON )

#Find OpendTect
list(APPEND CMAKE_MODULE_PATH "${OpendTect_DIR}/CMakeModules")
find_package( OpendTect )
get_filename_component( OpendTect_DIR "${OpendTect_DIR}" REALPATH )
get_filename_component( OD_BINARY_BASEDIR "${OD_BINARY_BASEDIR}" REALPATH )

#Setup the modules
set ( PLUGINS Tut uiTut )
OD_ADD_MODULES( plugins ${PLUGINS} )

#Add the plugins
foreach ( plugin ${PLUGINS} )
    get_filename_component( OD_${plugin}_RUNTIMEPATH "${OD_${PLUGIN}_RUNTIMEPATH}" REALPATH CACHE )
    list( APPEND RUNTIMEPATH ${OD_${plugin}_RUNTIMEPATH} )
endforeach()

list ( APPEND RUNTIMEPATH ${OpendTect_DIR}/${OD_EXEC_OUTPUT_RELPATH} )
list ( APPEND RUNTIMEPATH ${OD_BINARY_BASEDIR}/${OD_EXEC_OUTPUT_RELPATH} )
list ( REMOVE_DUPLICATES RUNTIMEPATH )

if( OD_CREATE_LAUNCHERS )
	set ( OD_LAUNCHERS od_main od_process_attrib od_process_volume )
    set ( ENV{DTECT_BINDIR} "${OpendTect_DIR}/${OD_EXEC_OUTPUT_RELPATH}" )
    foreach ( LAUNCHER ${OD_LAUNCHERS} )
	add_custom_target( ${LAUNCHER} ALL )
	create_target_launcher( ${LAUNCHER}
	    COMMAND "${LAUNCHER}${OD_EXECUTABLE_EXTENSION}"
	    RUNTIME_LIBRARY_DIRS "${RUNTIMEPATH}"
	    ENVIRONMENT
	    DTECT_APPL=${OpendTect_DIR}
	    WORK=${OpendTect_DIR} )
    endforeach()
endif( OD_CREATE_LAUNCHERS )

OD_WRITE_ALOFILES( ${CMAKE_CURRENT_BINARY_DIR} )
