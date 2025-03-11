#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#

cmake_minimum_required( VERSION 3.14.7 )
project( OpendTect_Tutorial_Plugin )

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set_property( GLOBAL PROPERTY USE_FOLDERS ON )

set( OD_SUBSYSTEM "tut_plugin" )
set( OpendTect_DIR "" CACHE PATH "OpendTect directory" )

#Setup output directory
option( OD_BUILD_LOCAL "Build in local directory" ON )

#Find OpendTect
list(APPEND CMAKE_MODULE_PATH "${OpendTect_DIR}/CMakeModules")
find_package( OpendTect )
get_filename_component( OpendTect_DIR "${OpendTect_DIR}" REALPATH )
get_filename_component( OD_BINARY_BASEDIR "${OD_BINARY_BASEDIR}" REALPATH )

#Setup the modules
set( PLUGINS Tut uiTut )
OD_ADD_MODULES( plugins ${PLUGINS} )

if( OD_CREATE_LAUNCHERS )
    list( APPEND RUNTIMEPATH ${OD_BINARY_BASEDIR}/${OD_EXEC_OUTPUT_RELPATH} )
    list( APPEND RUNTIMEPATH ${CMAKE_BINARY_DIR}/${OD_EXEC_OUTPUT_RELPATH} )
    list( REMOVE_DUPLICATES RUNTIMEPATH )

    set( OD_LAUNCHERS ${OD_MAIN_EXEC} ${OD_ATTRIB_EXECS} ${OD_VOLUME_EXECS} )
    set( ENV{DTECT_BINDIR} "${OD_BINARY_BASEDIR}/${OD_EXEC_OUTPUT_RELPATH}" )
    set( CMAKE_FOLDER "Launchers" )
    foreach ( LAUNCHER ${OD_LAUNCHERS} )
	add_custom_target( ${LAUNCHER} ALL )
	create_target_launcher( ${LAUNCHER}
	    COMMAND "${LAUNCHER}${OD_EXECUTABLE_EXTENSION}"
	    RUNTIME_LIBRARY_DIRS "${RUNTIMEPATH}"
	    WORKING_DIRECTORY "${OD_BINARY_BASEDIR}"
	    ENVIRONMENT
	    DTECT_APPL=${OD_BINARY_BASEDIR}
	    OD_USER_PLUGIN_DIR=${CMAKE_BINARY_DIR} )
    endforeach()
    unset( CMAKE_FOLDER )
endif( OD_CREATE_LAUNCHERS )

OD_WRITE_ALOFILES( "${CMAKE_BINARY_DIR}/plugins/${OD_PLFSUBDIR}" )
