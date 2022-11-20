#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#
# Main CMake file
#

cmake_minimum_required ( VERSION 3.14.7 )

set( MAIN_GIT_BRANCH main )

set( OpendTect_VERSION_MAJOR 7 )
set( OpendTect_VERSION_MINOR 1 )
if ( NOT DEFINED OpendTect_VERSION_PATCH OR OpendTect_VERSION_PATCH STREQUAL "" )
    set( OpendTect_VERSION_PATCH 0 )
endif()

if ( NOT DEFINED OpendTect_FULL_VERSION )
    set( OpendTect_FULL_VERSION
	 ${OpendTect_VERSION_MAJOR}.${OpendTect_VERSION_MINOR}.${OpendTect_VERSION_PATCH} )
endif()

if ( POLICY CMP0048 )
    cmake_policy( SET CMP0048 NEW )
endif()
if ( POLICY CMP0091 )
    cmake_policy( SET CMP0091 NEW )
endif()
project( "OpendTect"
    VERSION ${OpendTect_VERSION_MAJOR}.${OpendTect_VERSION_MINOR}.${OpendTect_VERSION_PATCH}
    DESCRIPTION "OpendTect" )

if ( POLICY CMP0011 )
    cmake_policy( SET CMP0011 NEW )
endif()
if( APPLE )
    if ( POLICY CMP0042 )
	cmake_policy( SET CMP0042 OLD )
    endif()
endif()
if ( POLICY CMP0057 )
    cmake_policy( SET CMP0057 NEW )
endif()
if ( POLICY CMP0074 )
    cmake_policy( SET CMP0074 NEW)
endif()
if ( POLICY CMP0110 )
    cmake_policy( SET CMP0110 NEW)
endif()

include( CTest )

set( CMAKE_CXX_STANDARD 14 )
set( CMAKE_CXX_STANDARD_REQUIRED True )
set_property( GLOBAL PROPERTY USE_FOLDERS ON )

set( OpendTect_DIR ${CMAKE_SOURCE_DIR} )
set( OD_BINARY_BASEDIR ${CMAKE_BINARY_DIR} )

if( NOT DEFINED CMAKE_BUILD_TYPE )
    set( CMAKE_BUILD_TYPE "Debug" CACHE STRING "Release or Debug" )
endif()

list( APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMakeModules" )

set( OD_SUBSYSTEM "od" )

option ( OD_NO_QT "Turn off all QT" OFF )
option ( OD_NO_OSG "Do not build anything with 3D visualisation" OFF )
option ( OD_NO_PROJ "Do not use proj library for CRS" OFF )
option ( BUILD_PROJ "Checkout and build proj in external" OFF )
set( PROJ_DIR "" CACHE PATH "Pre-built proj installation" )
option ( OD_ENABLE_BREAKPAD "Use breakpad" OFF )

include( OD_SetupOD )

# Note: the order of these modules *IS* important!
# Make sure dependent modules are *after* the ones that they depend upon.
set( OD_MODULES
	Basic Algo General
	Strat Network Batch
	Geometry EarthModel Well MMProc Seis
	NLA AttributeEngine Velocity
	VolumeProcessing PreStackProcessing Attributes
	MPEEngine EMAttrib WellAttrib
	uiBase uiTools uiCmdDriver uiFlatView uiIo uiSysAdm
	uiNLA uiSeis uiStrat uiEarthModel uiWell
	uiVelocity uiPreStackProcessing uiAttributes
	uiVolumeProcessing uiEMAttrib uiMPE uiViewer2D uiWellAttrib
)

if ( NOT OD_NO_OSG )
    list( APPEND OD_MODULES
	    visBase visSurvey uiOSG uiVis
	    uiODMain
    )
endif()

set( OD_SPECPROGS ODSeis ODvisBase uiBase )

OD_SETUP_EXTERNALS()
OD_ADD_EXTERNALS() # Once, in main scope
OD_ADD_MODULES( src ${OD_MODULES} )
include( plugins/CMakeLists.txt )
OD_ADD_MODULES( spec ${OD_SPECPROGS} )
OD_CREATE_PACKAGE_DEFINITION()

include( ODInternal )
OD_WRITE_MODDEPS( ${OD_BINARY_BASEDIR}/data )
OD_WRITE_FINDFILE()
OD_WRITE_TEST_PROJECT_DESC( ${OD_BINARY_BASEDIR}/test )

OD_ADD_PACKAGES_TARGET()
if ( WIN32 OR APPLE )
    OD_ADD_SIGNLIBRARIES_TARGET()
endif()

set( PLUGIN_DIR "" CACHE PATH "Path to eventual extra plugins" )
if ( EXISTS ${PLUGIN_DIR}/CMakeLists.txt )
    include ( ${PLUGIN_DIR}/CMakeLists.txt )
endif()

if ( DEFINED ${MISC_INSTALL_PREFIX} )
    OD_WRITE_ALOFILES( ${MISC_INSTALL_PREFIX}/plugins/${OD_PLFSUBDIR} )
else()
    OD_WRITE_ALOFILES( ${CMAKE_BINARY_DIR}/plugins/${OD_PLFSUBDIR} )
endif()


#OD_ADD_KEYWORD_TEST( strcmp strcmp )
OD_ADD_KEYWORD_TEST( "sqrt(" sqrt "Use Math::Sqrt() instead of sqrt" )
OD_ADD_KEYWORD_TEST( "atan2(" atan2 "Use Math::Atan2() instead of atan2" )
OD_ADD_REGEXP_TEST( "ArrPtrMan[^;]+=[^?;]*new\\\\\\\\s+" mAllocLargeVarLenArr
		     "ArrPtrMan<Type> var = new is not allowed. Use mAllocLargeVarLenArr instead." )
OD_ADD_LINEEND_TEST()
OD_ADD_EXIT_PROGRAM_TEST()
OD_ADD_LINT_TEST()

if ( UNIX )
    add_test( "PNGCompression" ${CMAKE_SOURCE_DIR}/dtect/FindCompactablePNGs.csh )
endif()

SETUP_QT_TRANSLATION( od )

OD_SETUP_TEST_FILTER()

if( BUILD_DOCUMENTATION )
    OD_BUILD_DOCUMENTATION()
endif()
if( BUILD_USERDOC )
    OD_BUILD_USERDOCUMENTATION()
endif()

if ( OD_BINARY_DEBUG_BASEDIR AND NOT ${CMAKE_BUILD_TYPE} STREQUAL "Debug" )
   set( OD_BINARY_DEBUG_BASEDIR "${OD_BINARY_DEBUG_BASEDIR}" CACHE PATH "Binary
	project in Debug mode" INTERNAL )
endif()
