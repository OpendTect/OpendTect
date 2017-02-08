#_______________________Pmake__________________________________________________
#
#       CopyRight:      dGB Beheer B.V.
#       Jan 2012        K. Tingdahl
#       SVN:		$Id$
#_______________________________________________________________________________

cmake_minimum_required ( VERSION 2.8.5 )

if ( CMAKE_VERSION VERSION_LESS 2.8.11 )
else()
    cmake_policy( SET CMP0020 OLD )
endif()

set( MAIN_GIT_BRANCH od6.2 )

set( OpendTect_VERSION_MAJOR 6 )
set( OpendTect_VERSION_MINOR 2 )
if ( NOT DEFINED OpendTect_VERSION_PATCH OR OpendTect_VERSION_PATCH STREQUAL "" )
    set( OpendTect_VERSION_PATCH 0 )
endif()

if ( NOT DEFINED OpendTect_FULL_VERSION )
    set( OpendTect_FULL_VERSION
	 ${OpendTect_VERSION_MAJOR}.${OpendTect_VERSION_MINOR}.${OpendTect_VERSION_PATCH} )
endif()

if ( CMAKE_VERSION VERSION_LESS 3.0.0 )
    project( "OpendTect-${OpendTect_VERSION_MAJOR}_${OpendTect_VERSION_MINOR}" )
else()
    cmake_policy( SET CMP0042 OLD )
    cmake_policy( SET CMP0048 NEW )
    project( "OpendTect-${OpendTect_VERSION_MAJOR}_${OpendTect_VERSION_MINOR}"
	    VERSION ${OpendTect_VERSION_MAJOR}.${OpendTect_VERSION_MINOR}.${OpendTect_VERSION_PATCH} )
endif()



include( CTest )

set( OpendTect_DIR ${CMAKE_SOURCE_DIR} )
set( OD_BINARY_BASEDIR ${CMAKE_BINARY_DIR} )

if( NOT DEFINED CMAKE_BUILD_TYPE )
    set( CMAKE_BUILD_TYPE "Debug" CACHE STRING "Release or Debug" )
endif()

set( OD_DGB_DIR "" CACHE PATH "Place where closed source part is found" )

list( APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMakeModules" )

set( OD_SUBSYSTEM "od" )

include( OD_SetupOD )
include( ODInternal )

#Add osgGeo
OD_ADD_OSGGEO()


set( OD_MODULES
	Basic Algo General
	Strat Network Batch
	Geometry EarthModel Well MMProc Seis
	NLA AttributeEngine Velocity
	VolumeProcessing PreStackProcessing Attributes
	EMAttrib MPEEngine WellAttrib
	uiBase uiTools uiCmdDriver uiFlatView uiIo uiSysAdm
	uiNLA uiSeis uiStrat uiEarthModel uiWell
	uiVelocity uiPreStackProcessing uiAttributes
	uiVolumeProcessing uiEMAttrib uiMPE uiViewer2D uiWellAttrib
	visBase visSurvey uiOSG uiVis
	uiODMain
)

set( OD_PLUGINS
	ExpAttribs
	GLCM
	GMT
	Hello
	HorizonAttrib
	Madagascar
	MadagascarAttribs
	MATLABLink
	Tut
	VoxelConnectivityFilter
	uiDPSDemo
	uiExpAttribs
	uiGLCM
	uiGMT
	uiGoogleIO
	uiSEGY
	uiHello
	uiHorizonAttrib
	uiImpGPR
	uiMadagascar
	uiMadagascarAttribs
	uiMATLABLink
	uiPresentationMaker
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

set( PLUGIN_DIR ""  CACHE PATH "Path to eventual extra plugins" )
if ( EXISTS ${PLUGIN_DIR}/CMakeLists.txt )
    include ( ${PLUGIN_DIR}/CMakeLists.txt )
endif()

OD_WRITE_ALOFILES( ${MISC_INSTALL_PREFIX}/plugins/${OD_PLFSUBDIR} )

#OD_ADD_KEYWORD_TEST( strcmp strcmp )
OD_ADD_KEYWORD_TEST( "sqrt(" sqrt "Use Math::Sqrt() instead of sqrt" )
OD_ADD_KEYWORD_TEST( "atan2(" atan2 "Use Math::Atan2() instead of atan2" )
OD_ADD_REGEXP_TEST( "ArrPtrMan[^;]+=[^?;]*new\\\\s+" mAllocLargeVarLenArr
		     "ArrPtrMan<Type> var = new is not allowed. Use mAllocLargeVarLenArr instead." )
OD_ADD_LOCAL_STATIC_TEST()
OD_ADD_LINEEND_TEST()
OD_ADD_EXIT_PROGRAM_TEST()

if ( UNIX )
    add_test( "PNGCompression" ${CMAKE_SOURCE_DIR}/dtect/FindCompactablePNGs.csh )
endif()

SETUP_QT_TRANSLATION( od )

OD_SETUP_TEST_FILTER()

if( BUILD_DOCUMENTATION )
    OD_BUILD_DOCUMENTATION()
endif()

OD_ADD_PACKAGES_TARGET()

#Avoid warnings if specified on command line
set ( DUMMY ${ISTAG} ${BRANCH} ${CMAKE_PDB_OUTPUT_DIRECTORY} ${OD_RUN_LMSTRIP}
	    ${COINDIR} ${BUILD_dGBPDF3D} ${PDF3D_ROOT} )
