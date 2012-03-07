#_______________________Pmake__________________________________________________
#
#       CopyRight:      dGB Beheer B.V.
#       Jan 2012        K. Tingdahl
#       RCS :           $Id: RootMakefile,v 1.4 2010-09-28 07:21:07 cvsbert
#_______________________________________________________________________________

cmake_minimum_required (VERSION 2.6)
PROJECT(OpendTect)

SET ( OpendTect_DIR ${CMAKE_SOURCE_DIR} )
SET( OD_BINARY_BASEDIR ${OpendTect_DIR} )


IF ( NOT DEFINED CMAKE_BUILD_TYPE )
    SET(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Release or Debug" )
ENDIF()

SET(OD_DGB_DIR "" CACHE PATH
	"Place where closed source part is found")

SET (OpendTect_VERSION_MAJOR 4)
SET (OpendTect_VERSION_MINOR 3)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMakeModules")


INCLUDE ( OD_SetupOD )

SET (OD_SUBSYSTEM ${OD_CORE_SUBSYSTEM} )

SET(OD_MODULES
	Basic Algo Database General
	Strat Network Batch Usage
	Geometry EarthModel MMProc Seis
	NLA AttributeEngine Velocity
	VolumeProcessing PreStackProcessing Attributes
	EMAttrib MPEEngine Well WellAttrib
	uiBase uiTools uiFlatView uiIo uiSysAdm
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
	GoogleTranslate
	GMT
	Hello
	HorizonAttrib
	Madagascar
	MadagascarAttribs
	Tut
	VoxelConnectivityFilter
	uiBouncy
	uiDPSDemo
	uiExpAttribs
	uiGapDecon
	uiGoogleIO
	uiGrav
	uiHello
	uiHorizonAttrib
	uiMadagascar
	uiMadagascarAttribs
	uiQtApp
	uiTut
	uiTutMadagascar
	uiVoxelConnectivityFilter
	uiPreStackViewer )

OD_ADD_MODULES( src ${OD_MODULES} )
OD_ADD_MODULES( plugins ${OD_PLUGINS} )

OD_WRITE_MODDEPS()
OD_WRITE_FINDFILE()
OD_WRITE_ALOFILES()

IF( BUILD_DOCUMENTATION )
    OD_BUILD_DOCUMENTATION()
ENDIF()
