#!/bin/csh
#(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# Description:  CMake script to define base package variables
# Author:       Nageswara
# Date:         August 2012
#RCS:           $Id: basedefs.cmake,v 1.2 2012/09/11 12:25:44 cvsnageswara Exp $

#OpenDtect libraries
SET( LIBLIST Algo AttributeEngine Attributes Basic Batch Database EarthModel General
	     Geometry MMProc MPEEngine Network NLA Seis Strat Velocity VolumeProcessing
	     PreStackProcessing EMAttrib SoOD Well WellAttrib uiAttributes uiBase uiCoin
	     uiEarthModel uiEMAttrib uiFlatView uiIo uiMPE uiNLA uiODMain uiSeis uiStrat
	     uiTools uiPreStackProcessing uiVelocity uiViewer2D uiVis uiVolumeProcessing
	     uiWell uiWellAttrib uiSysAdm Usage visBase visSurvey )

SET( EXECLIST od_cbvs_browse od_glxinfo od_ivfileviewer lmhostid 
	      od_main od_sysadmmain od_process_attrib od_process_attrib_em
	      od_process_prestack od_process_segyio od_process_time2depth
	      od_process_velocityconv od_process_volume od_ProgressViewer od_DispMsg
	      od_FileBrowser od_SEGYExaminer od_SeisMMBatch od_ClusterProc
	      od_process_2dgrid od_remexec od_remoteservice od_stratamp od_isopach
	      od_ReportIssue od_uiReportIssue )
SET( PLUGINS HorizonAttrib GapDecon VoxelConnectivityFilter
	     uiHorizonAttrib uiPreStackViewer Annotations uiGapDecon 
	     uiGoogleIO GoogleTranslate CmdDriver uiVoxelConnectivityFilter )
##TODO should be exist in inst/bin/plf directory
#od_glxinfo lmhostid 
#Only for windows base package
SET( WINEXECLIST od_start_dtect od_main_console unzip )
SET( SPECFILES .exec_prog .init_dtect .init_dtect_user install .lic_inst_common
	       .lic_start_common mk_datadir .setappl.sh .start_dtect setup.od *.txt )

#Third party libraries
SET( LUXQTLIBS libQtCore.so.4 libQtGui.so.4 libQtOpenGL.so.4 libQtSql.so.4 libQtXml.so.4
	       libQtNetwork.so.4 libQt3Support.so.4 )
#SET( LUXCOINLIBS libCoin.so.6? libSoQt.so.2? libsimage.so.2? )
SET( LUXCOINLIBS libCoin.so.6 libSoQt.so.2 libsimage.so.2 )
SET( LUXOSGLIBS "" )

SET( MACQTLIBS libQtCore.4.dylib libQtGui.4.dylib libQtOpenGL.4.dylib libQtSql.4.dylib
	       libQtXml.4.dylib libQtNetwork.4.dylib libQt3Support.4.dylib )
SET( MACCOINLIBS libCoin.6?.dylib libsimage.2?.dylib libSoQt.2?.dylib libSimVoleon.4?.dylib )
SET( MACOSGLIBS "" )

SET( WINQTLIBS "" )
SET( WINCOINLIBS "" )
SET( WINOSGLIBS "" )

SET( PACK "base" )
