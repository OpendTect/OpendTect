#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#
# Main CMake file
#

cmake_minimum_required ( VERSION 3.24.0 )

set( MAIN_GIT_BRANCH od8.0 )

set( OpendTect_VERSION_MAJOR 8 )
set( OpendTect_VERSION_MINOR 0 )
if ( NOT DEFINED OpendTect_VERSION_PATCH OR OpendTect_VERSION_PATCH STREQUAL "" )
    set( OpendTect_VERSION_PATCH 0 )
endif()

if ( NOT DEFINED OpendTect_FULL_VERSION )
    set( OpendTect_FULL_VERSION "2025.DEV" )
endif()
if ( NOT DEFINED OpendTect_INST_DIR )
    set( OpendTect_INST_DIR "2025" )
endif()

if ( POLICY CMP0048 )
    cmake_policy( SET CMP0048 NEW )
endif()
if ( POLICY CMP0091 )
    cmake_policy( SET CMP0091 NEW )
endif()
project( "OpendTect-${OpendTect_VERSION_MAJOR}_${OpendTect_VERSION_MINOR}"
    VERSION ${OpendTect_VERSION_MAJOR}.${OpendTect_VERSION_MINOR}.${OpendTect_VERSION_PATCH}
    DESCRIPTION "OpendTect" )

if ( POLICY CMP0011 )
    cmake_policy( SET CMP0011 NEW )
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
if ( POLICY CMP0144 )
    cmake_policy( SET CMP0144 NEW )
endif()
if ( POLICY CMP0177 )
    cmake_policy( SET CMP0177 NEW )
endif()

include( CTest )

set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_CXX_STANDARD_REQUIRED True )
set_property( GLOBAL PROPERTY USE_FOLDERS ON )
#if ( CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_DEBUG_POSTFIX )
#    set( CMAKE_DEBUG_POSTFIX "d" )
#endif()

set( OpendTect_DIR ${CMAKE_SOURCE_DIR} )
set( OD_BINARY_BASEDIR ${CMAKE_BINARY_DIR} )

list( APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMakeModules" )

set( OD_SUBSYSTEM "od" )

option ( OD_NO_QT "Turn off all QT" OFF )
option ( OD_USE_QOPENGL "Use QOpenGL classes instead of QGL" ON )
option ( OD_NO_QPRINTSUPPORT "Do not use Qt's PrintSupport module" OFF )
option ( OD_NO_QSQL "Do not use Qt's SQL module" ON )
option ( OD_NO_OSG "Do not build anything with 3D visualisation" OFF )
option ( OD_NO_PROJ "Do not use proj library for CRS" OFF )
option ( OD_ENABLE_BREAKPAD "Use breakpad" OFF )

include( OD_SetupOD )

# Note: the order of these modules *IS* important!
# Make sure dependent modules are *after* the ones that they depend upon.
set( OD_MODULES
	Basic Algo General
	Geometry Strat Network Batch
	EarthModel Well MMProc Seis
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

set( OD_SPECPROGS ODSeis ODvisBase ODuiBase )

OD_SETUP_EXTERNALS()
OD_ADD_EXTERNALS() # Once, in main scope
OD_ADD_MODULES( src ${OD_MODULES} )
include( plugins/CMakeLists.txt )
OD_ADD_MODULES( spec ${OD_SPECPROGS} )

include( ODInternal )
OD_WRITE_MODDEPS( "${CMAKE_BINARY_DIR}/${OD_DATA_INSTALL_RELPATH}" )
OD_WRITE_FINDFILE()
OD_WRITE_TEST_PROJECT_DESC( ${OD_BINARY_BASEDIR}/test )

OD_ADD_PACKAGES_TARGET()
if ( WIN32 OR APPLE )
    OD_ADD_SIGNLIBRARIES_TARGET()
endif()
set( DEPLOYEMENT_SCRIPT "" CACHE FILEPATH "Filepath to the cmake deployement script" )
if ( EXISTS "${DEPLOYEMENT_SCRIPT}" )
    include( "${DEPLOYEMENT_SCRIPT}" )
    OD_ADD_DEPLOYEMENT_TARGETS()
endif()

set( PLUGIN_DIR "" CACHE PATH "Path to eventual extra plugins" )
if ( EXISTS ${PLUGIN_DIR}/CMakeLists.txt )
    file( REAL_PATH "${PLUGIN_DIR}" PLUGIN_DIR )
    include ( ${PLUGIN_DIR}/CMakeLists.txt )
endif()

OD_WRITE_ALOFILES( "${OD_BINARY_BASEDIR}/${MISC_INSTALL_PREFIX}/plugins/${OD_PLFSUBDIR}" )

#OD_ADD_KEYWORD_TEST( strcmp strcmp )
OD_ADD_KEYWORD_TEST( "sqrt(" sqrt "Use Math::Sqrt() instead of sqrt" )
OD_ADD_KEYWORD_TEST( "atan2(" atan2 "Use Math::Atan2() instead of atan2" )
OD_ADD_REGEXP_TEST( "ArrPtrMan[^;]+=[^?;]*new\\\\\\\\s+" mAllocLargeVarLenArr
		     "ArrPtrMan<Type> var = new is not allowed. Use mAllocLargeVarLenArr instead." )
OD_ADD_LINEEND_TEST()
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
