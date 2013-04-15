#_______________________Pmake__________________________________________________
#
#       CopyRight:      dGB Beheer B.V.
#       Jan 2012        K. Tingdahl
#       SVN:		$Id$
#_______________________________________________________________________________

cmake_minimum_required (VERSION 2.8)

set( OpendTect_VERSION_MAJOR 4 )
set( OpendTect_VERSION_MINOR 5 )
set( OpendTect_VERSION_DETAIL 0 )
set( OpendTect_VERSION_PATCH a )

PROJECT( "OpendTect" )

include( CTest )

set( OpendTect_DIR ${CMAKE_SOURCE_DIR} )
set( OD_BINARY_BASEDIR ${CMAKE_BINARY_DIR} )

if( NOT DEFINED CMAKE_BUILD_TYPE )
    set( CMAKE_BUILD_TYPE "Debug" CACHE STRING "Release or Debug" )
endif()

set( OD_DGB_DIR "" CACHE PATH "Place where closed source part is found" )

list( APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMakeModules" )

include( OD_SetupOD )
include( ODInternal )

set( OD_SUBSYSTEM ${OD_CORE_SUBSYSTEM} )

set( OD_MODULES
	Basic Algo General Database
	Strat Network Batch Usage
	Geometry EarthModel MMProc Seis
	NLA AttributeEngine Velocity
	VolumeProcessing PreStackProcessing Attributes
	EMAttrib MPEEngine Well WellAttrib
	uiBase uiTools uiCmdDriver uiFlatView uiIo uiSysAdm
	uiNLA uiSeis uiStrat uiEarthModel uiWell
	uiVelocity uiVolumeProcessing uiPreStackProcessing
	uiAttributes uiEMAttrib uiMPE uiViewer2D uiWellAttrib
	SoOD visBase visSurvey uiCoin uiVis
	uiODMain
)

set( OD_PLUGINS
	Annotations
	Bouncy
	ExpAttribs
	GapDecon
	GMT
	GoogleTranslate
	Hello
	HorizonAttrib
	Madagascar
	MadagascarAttribs
	MatlabLink
	TextureAttrib
	Tut
	VoxelConnectivityFilter
	uiBouncy
	uiDPSDemo
	uiExpAttribs
	uiGapDecon
	uiGMT
	uiGoogleIO
	uiGrav
	uiHello
	uiHorizonAttrib
	uiImpGPR
	uiMadagascar
	uiMadagascarAttribs
	uiMatlabLink
	uiTextureAttrib
	uiTut
	uiTutMadagascar
	uiVoxelConnectivityFilter
	uiPreStackViewer
	CmdDriver
)

set( OD_SPECPROGS ODSeis visBase )
OD_CREATE_DEVEL_PACKAGE_DEFINITION()

OD_ADD_MODULES( src ${OD_MODULES} )
OD_ADD_MODULES( plugins ${OD_PLUGINS} )
OD_ADD_MODULES( spec ${OD_SPECPROGS} )

OD_WRITE_MODDEPS( ${OD_BINARY_BASEDIR}/data )
OD_WRITE_FINDFILE()
OD_WRITE_TEST_PROJECT_DESC( ${OD_BINARY_BASEDIR}/test )
OD_WRITE_ALOFILES( ${OD_BINARY_BASEDIR}/plugins/${OD_PLFSUBDIR} )

#OD_ADD_KEYWORD_TEST( strcmp strcmp )
OD_ADD_KEYWORD_TEST( "sqrt(" sqrt )
OD_ADD_LINEEND_TEST()
if( NOT DEFINED APPLE )
    OD_ADD_SVNPROP_TEST()
endif()

set ( OD_TESTS_IGNORE_CONTINUOUS SVNPropertyTest )
OD_SETUP_TEST_FILTER()

if( BUILD_DOCUMENTATION )
    OD_BUILD_DOCUMENTATION()
endif()

#Avoid warnings if specified on command line
set ( DUMMY ${ISTAG} ${BRANCH} )
