#!/bin/csh
#(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# Description:  CMake script to define devel package variables
# Author:       Nageswara
# Date:         August 2012
#RCS:           $Id$

#OpenDtect libraries
SET( LIBLIST AttributeEngine Algo Attributes Basic Database EarthModel General
	     Geometry MMProc MPEEngine Network NLA Seis Strat Velocity VolumeProcessing
	     PreStackProcessing EMAttrib SoOD Well WellAttrib uiAttributes uiBase
	     uiCoin uiEarthModel uiEMAttrib uiFlatView uiIo uiMPE uiNLA uiODMain
	     uiSeis uiStrat uiTools uiPreStackProcessing uiVelocity uiViewer2D
	     uiVis uiVolumeProcessing uiWell uiWellAttrib uiSysAdm Usage visBase
	     visSurvey uiCmdDriver )
SET( INCLIBLIST ${LIBLIST} Prog )
SET( SRCLIBLIST ${LIBLIST} Batch )

SET( EXECLIST od_cbvs_browse od_glxinfo od_ivfileviewer lmhostid 
	      od_main od_sysadmmain od_process_attrib od_process_attrib_em
	      od_process_prestack od_process_segyio od_process_time2depth
	      od_process_velocityconv od_process_volume od_ProgressViewer od_DispMsg
	      od_FileBrowser od_SEGYExaminer od_SeisMMBatch od_ClusterProc
	      od_process_2dgrid od_remexec od_remoteservice od_stratamp od_isopach
	      od_ReportIssue od_uiReportIssue )
SET( PLUGINS HorizonAttrib GapDecon VoxelConnectivityFilter
	     uiHorizonAttrib uiPreStackViewer Annotations uiGapDecon 
	     uiGoogleIO GoogleTranslate CmdDriver uiVoxelConnectivityFilter
	     Bouncy ExpAttribs uiBouncy uiExpAttribs
	     Hello uiHello Tut uiTut uiDPSDemo )
SET( PMAKESTUFF Makefile RootMakefile make.od.Defaults make.od.Dirs 
		make.od.Vars base )
#SET( PMAKESTUFF Makefile ModDeps.od RootMakefile make.od.Defaults make.od.Dirs 
#		make.od.Vars make.od.ModDeps base )
SET( SPECSOURCES EarthModel ODGeneral ODSeis visBase )
#SET( SPECSOURCES Makefile.od EarthModel ODGeneral ODSeis visBase )
SET( PACK "devel" )
