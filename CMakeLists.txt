#_______________________Pmake__________________________________________________
#
#       CopyRight:      dGB Beheer B.V.
#       Jan 2012        K. Tingdahl
#       RCS :           $Id: RootMakefile,v 1.4 2010-09-28 07:21:07 cvsbert
#_______________________________________________________________________________

cmake_minimum_required (VERSION 2.6)
PROJECT(OpendTect)

SET(CMAKE_BUILD_TYPE "" CACHE STRING "Release or Debug" )
SET(OD_DGB_DIR "" CACHE PATH
	"Place where closed source part is found")

SET (OpendTect_VERSION_MAJOR 4)
SET (OpendTect_VERSION_MINOR 3)

INCLUDE ( CMakeModules/ODQtUtils.cmake REQUIRED )
INCLUDE ( CMakeModules/ODCoinUtils.cmake REQUIRED )
INCLUDE ( CMakeModules/ODOsgUtils.cmake REQUIRED )
INCLUDE ( CMakeModules/ODPlatformUtils.cmake REQUIRED )
INCLUDE ( CMakeModules/ODMacroUtils.cmake REQUIRED )
INCLUDE ( CMakeModules/ODUtils.cmake REQUIRED )

SET(MODULES
	Basic
	Algo
	Database
	General
	Usage
	Strat
	Network
	Batch
	Geometry
	EarthModel
	MMProc
	Seis
	NLA
	AttributeEngine
	Velocity
	VolumeProcessing
	PreStackProcessing
	Attributes
	EMAttrib
	MPEEngine
	Well
	WellAttrib
	uiBase
	uiTools
	uiFlatView
	uiIo
	uiSysAdm
	uiNLA
	uiSeis
	uiStrat
	uiEarthModel
	uiWell
	uiVelocity
	uiVolumeProcessing
	uiPreStackProcessing
	uiAttributes
	uiEMAttrib
	uiMPE
	uiViewer2D
	uiWellAttrib
	SoOD
	visBase
	visSurvey
	uiCoin
	uiVis
	uiODMain
)

FOREACH( OD_MODULE_NAME ${MODULES} )
    add_subdirectory( src/${OD_MODULE_NAME} )
ENDFOREACH()

ADD_SUBDIRECTORY(plugins)
IF(OD_DGB_DIR)
    INCLUDE(${OD_DGB_DIR}/ODDGB.cmake REQUIRED)
ENDIF(OD_DGB_DIR)
