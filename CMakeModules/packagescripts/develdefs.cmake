#(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# Description:  CMake script to define devel package variables
# Author:       Nageswara
# Date:         August 2012
#RCS:           $Id$

#OpenDtect libraries
#Mantain the same module and plugins order
# which is in ${CMAKE_SOURCE_DIR}/CMakeLists.txt to avoid compiler errors.
set( LIBLIST Basic Algo Database General
	     Strat Network Usage
	     Geometry EarthModel MMProc Seis
	     NLA AttributeEngine Velocity
	     VolumeProcessing PreStackProcessing Attributes
	     EMAttrib MPEEngine Well WellAttrib
	     uiBase uiTools uiCmdDriver uiFlatView uiIo uiSysAdm
	     uiNLA uiSeis uiStrat uiEarthModel uiWell
	     uiVelocity uiVolumeProcessing uiPreStackProcessing
	     uiAttributes uiEMAttrib uiMPE uiViewer2D uiWellAttrib
	     SoOD visBase visSurvey uiCoin uiVis uiODMain )

SET( INCLIBLIST ${LIBLIST} Prog )
SET( SRCLIBLIST ${LIBLIST} Batch )

SET( EXECLIST od_cbvs_browse od_glxinfo od_ivfileviewer lmhostid 
	      od_main od_sysadmmain od_process_attrib od_process_attrib_em
	      od_process_prestack od_process_segyio od_process_time2depth
	      od_process_velocityconv od_process_volume od_ProgressViewer od_DispMsg
	      od_FileBrowser od_SEGYExaminer od_SeisMMBatch od_ClusterProc
	      od_process_2dgrid od_remexec od_remoteservice od_stratamp od_isopach
	      od_ReportIssue od_uiReportIssue )

SET( PLUGINS Annotations Bouncy ExpAttribs GapDecon GMT GoogleTranslate Hello
	     HorizonAttrib Madagascar MadagascarAttribs TextureAttrib Tut
	     VoxelConnectivityFilter uiBouncy uiDPSDemo uiExpAttribs uiGapDecon uiGMT
	     uiGoogleIO uiGrav uiHello uiHorizonAttrib uiImpGPR uiMadagascar
	     uiMadagascarAttribs uiTextureAttrib uiTut uiTutMadagascar
	     uiVoxelConnectivityFilter uiPreStackViewer CmdDriver )
SET( PMAKESTUFF Makefile RootMakefile make.od.Defaults make.od.Dirs 
		make.od.Vars base )
SET( SPECSOURCES EarthModel ODGeneral ODSeis visBase )
SET( TESTS testBasic testGeneral )
SET( PACK "devel" )
