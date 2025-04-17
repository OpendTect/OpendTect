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

if ( POLICY CMP0144 )
    cmake_policy( SET CMP0144 NEW )
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set_property( GLOBAL PROPERTY USE_FOLDERS ON )

set( OD_SUBSYSTEM "tut_plugin" )
set( OpendTect_DIR "" CACHE PATH "OpendTect directory" )

#Setup output directory
option( OD_BUILD_LOCAL "Build in local directory" ON )

#Find OpendTect
if ( EXISTS "${OD_BINARY_BASEDIR}/CMakeModules/FindOpendTect.cmake" )
    list( APPEND CMAKE_MODULE_PATH "${OD_BINARY_BASEDIR}/CMakeModules" )
elseif( EXISTS "${OD_BINARY_BASEDIR}/Contents/Resources/CMakeModules/FindOpendTect.cmake" )
    list( APPEND CMAKE_MODULE_PATH "${OD_BINARY_BASEDIR}/Contents/Resources/CMakeModules" )
else()
    message( FATAL_ERROR "Could not find OpendTect CMakeModules" )
endif()

if ( NOT "${OpendTect_DIR}" STREQUAL "${OD_BINARY_BASEDIR}" )
    if ( IS_DIRECTORY "${OpendTect_DIR}/CMakeModules" )
        list( APPEND CMAKE_MODULE_PATH "${OpendTect_DIR}/CMakeModules" )
    elseif ( IS_DIRECTORY "${OpendTect_DIR}/Contents/Resources/CMakeModules" )
        list( APPEND CMAKE_MODULE_PATH "${OpendTect_DIR}/Contents/Resources/CMakeModules" )
    else()
        message( FATAL_ERROR "Could not find OpendTect CMakeModules" )
    endif()
endif()

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

OD_ADD_EXTERNALS()
link_directories( "${CMAKE_BINARY_DIR}/${OD_LIBRARY_DIRECTORY}" )
OD_ADD_MODULES( plugins ${PLUGINS} )

if( OD_CREATE_LAUNCHERS )
    list( APPEND RUNTIMEPATH "${OD_BINARY_BASEDIR}/${OD_LIBRARY_DIRECTORY}" )
    list( APPEND RUNTIMEPATH "${CMAKE_BINARY_DIR}/${OD_LIBRARY_DIRECTORY}" )
    list( REMOVE_DUPLICATES RUNTIMEPATH )

    set( OD_LAUNCHERS ${OD_MAIN_EXEC} ${OD_ATTRIB_EXECS} ${OD_VOLUME_EXECS} )
    set( ENV{DTECT_BINDIR} "${OD_BINARY_BASEDIR}" )
    set( CMAKE_FOLDER "Launchers" )
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
    unset( CMAKE_FOLDER )
endif( OD_CREATE_LAUNCHERS )

OD_WRITE_ALOFILES( "${CMAKE_BINARY_DIR}/${MISC_INSTALL_PREFIX}/plugins/${OD_PLFSUBDIR}" )
