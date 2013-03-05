#_______________________Pmake__________________________________________________
#
#       CopyRight:      dGB Beheer B.V.
#       Jan 2012        K. Tingdahl
#       SVN:		$Id$
#_______________________________________________________________________________

cmake_minimum_required (VERSION 2.8)

SET( OpendTect_VERSION_MAJOR 4 )
SET( OpendTect_VERSION_MINOR 5 )
SET( OpendTect_VERSION_DETAIL 0 )
SET( OpendTect_VERSION_PATCH a )

PROJECT( "OpendTect" )

INCLUDE( CTest )

SET( OpendTect_DIR ${CMAKE_SOURCE_DIR} )
SET( OD_BINARY_BASEDIR ${CMAKE_BINARY_DIR} )

IF( NOT DEFINED CMAKE_BUILD_TYPE )
    SET( CMAKE_BUILD_TYPE "Debug" CACHE STRING "Release or Debug" )
ENDIF()

SET( OD_DGB_DIR "" CACHE PATH "Place where closed source part is found" )

LIST( APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMakeModules" )

INCLUDE( OD_SetupOD )
INCLUDE( ODInternal )

SET( OD_SUBSYSTEM ${OD_CORE_SUBSYSTEM} )

SET( OD_MODULES
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

SET( OD_PLUGINS
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
	uiTextureAttrib
	uiTut
	uiTutMadagascar
	uiVoxelConnectivityFilter
	uiPreStackViewer
	CmdDriver
)

SET( OD_SPECPROGS ODSeis )

OD_ADD_MODULES( src ${OD_MODULES} )
OD_ADD_MODULES( plugins ${OD_PLUGINS} )
OD_ADD_MODULES( spec ${OD_SPECPROGS} )
OD_ADD_TEST_MODULES( Basic General PreStackProcessing )

OD_WRITE_MODDEPS( ${OD_BINARY_BASEDIR}/data )
OD_WRITE_FINDFILE()
OD_WRITE_TEST_PROJECT_DESC( ${OD_BINARY_BASEDIR}/test )
OD_WRITE_ALOFILES( ${OD_BINARY_BASEDIR}/plugins/${OD_PLFSUBDIR} )

#OD_ADD_KEYWORD_TEST( strcmp strcmp )
OD_ADD_KEYWORD_TEST( "sqrt(" sqrt )
OD_ADD_LINEEND_TEST()
IF( NOT DEFINED APPLE )
    OD_ADD_SVNPROP_TEST()
endif()

IF( BUILD_DOCUMENTATION )
    OD_BUILD_DOCUMENTATION()
ENDIF()
