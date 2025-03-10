#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#

cmake_minimum_required( VERSION 3.14.7 )

set( PRODUCT_NAME "OpendTect Tutorial Plugin" )
set( PROJECT_VERSION "0.0.0" )
set( VENDOR_NAME "My New Company" )
set( PACKAGE_NAME "tutpackage" )

project( ${PRODUCT_NAME}
	 VERSION ${PROJECT_VERSION} )

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

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
foreach( PLUGIN ${PLUGINS} )
    set( ${PLUGIN}_VERSION_MAJOR 1 )
    set( ${PLUGIN}_VERSION_MINOR 2 )
    set( ${PLUGIN}_VERSION_PATCH 3 )
    set( ${PLUGIN}_PRODUCT_NAME ${PRODUCT_NAME} )
    set( ${PLUGIN}_PACKAGE ${PACKAGE_NAME} )
    # Mapping between each plugin and its version, product name and package
    # Made plugin specific
endforeach()

OD_ADD_MODULES( plugins ${PLUGINS} )

list( APPEND RUNTIMEPATH "${OD_BINARY_BASEDIR}/${OD_LIBRARY_DIRECTORY}" )
list( APPEND RUNTIMEPATH "${CMAKE_BINARY_DIR}/${OD_LIBRARY_DIRECTORY}" )
list( REMOVE_DUPLICATES RUNTIMEPATH )

if( OD_CREATE_LAUNCHERS )
    set( OD_LAUNCHERS od_main od_process_attrib od_process_volume )
    set( ENV{DTECT_BINDIR} "${OD_BINARY_BASEDIR}" )
    foreach ( LAUNCHER ${OD_LAUNCHERS} )
	add_custom_target( ${LAUNCHER} ALL )
	create_target_launcher( ${LAUNCHER}
	    COMMAND ${LAUNCHER}
	    RUNTIME_LIBRARY_DIRS "${RUNTIMEPATH}"
	    WORKING_DIRECTORY "${OD_BINARY_BASEDIR}"
	    ENVIRONMENT
	    DTECT_APPL=${OD_BINARY_BASEDIR}
	    OD_USER_PLUGIN_DIR=${CMAKE_BINARY_DIR} )
    endforeach()
endif( OD_CREATE_LAUNCHERS )

OD_WRITE_ALOFILES( "${CMAKE_BINARY_DIR}/plugins/${OD_PLFSUBDIR}" )
