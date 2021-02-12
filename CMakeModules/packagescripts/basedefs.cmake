#(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# Description:  CMake script to define base package variables
# Author:       Nageswara
# Date:         August 2012
#RCS:           $Id$

#OpenDtect libraries
set( LIBLIST Algo AttributeEngine Attributes Basic Batch EarthModel General
	     Geometry MMProc MPEEngine Network NLA Seis Strat Velocity VolumeProcessing
	     PreStackProcessing EMAttrib ExpAttribs Well WellAttrib uiAttributes uiBase uiOSG
	     uiEarthModel uiEMAttrib uiExpAttribs uiFlatView uiIo uiMPE uiNLA uiODMain uiSeis uiStrat
	     uiTools uiPreStackProcessing uiVelocity uiViewer2D uiVis uiVolumeProcessing
	     uiWell uiWellAttrib uiSysAdm visBase visSurvey uiCmdDriver )

set( EXECLIST od_cbvs_browse od_osgfileviewer
	      od_main od_sysadmmain od_process_attrib od_process_attrib_em
	      od_process_prestack od_process_segyio od_process_time2depth
	      od_process_velocityconv od_process_volume
	      od_ProgressViewer od_DispMsg od_ImageViewer
	      od_FileBrowser od_SEGYExaminer od_SeisMMBatch od_ClusterProc
	      od_process_2dgrid od_remexec od_remoteservice od_stratamp od_isopach
	      od_uiReportIssue od_gmtexec od_madexec
	      od_process_2dto3d od_BatchHosts od_copy_seis od_PreStackMMBatch
	      od_mmptestbatch od_MMPTestLaunch od_Edit_Survey od_Manage_Surveys od_DBMan )

if ( NOT APPLE )
    set( EXECLIST ${EXECLIST} od_glxinfo )
endif()


set( PLUGINS VoxelConnectivityFilter uiPreStackViewer
	     uiGoogleIO CmdDriver uiVoxelConnectivityFilter
	     GMT uiGMT uiImpGPR Madagascar uiMadagascar
	     MadagascarAttribs uiMadagascarAttribs GLCM uiGLCM uiSEGYTools uiSEGY uiMMPTest
	     uiPresentationMaker CRS uiCRS CEEMDAttrib uiCEEMDAttrib )
if( NOT MATLAB_DIR STREQUAL "" )
    set( PLUGINS ${PLUGINS} MATLABLink uiMATLABLink )
endif()

if( INCLUDE_ODHDF5 STREQUAL "YES" )
    set( PLUGINS ${PLUGINS} ODHDF5 uiODHDF5  )
endif()

#Only for windows base package
set( WINEXECLIST od_main_console od_runinst )

#No need to include shell scripts in windows base package.
set( TXTFILES GNU_GENERAL_PUBLIC_LICENSE.txt INSTALL.txt LICENSE.txt )
if( WIN32 )
    set( SPECFILES ${TXTFILES} )
    set( ODSCRIPTS process_dumpfile.cmd od_external.bat )
else()
    set( SPECFILES .exec_prog .init_dtect .init_dtect_user install
		   .lic_inst_common .lic_start_common mk_datadir .start_dtect
		   setup.od odinit.matlab )
    set( SPECFILES ${SPECFILES} ${TXTFILES} )
    set( ODSCRIPTS od_* mksethdir macterm.in process_dumpfile.sh init_dtect_GL )
    list( REMOVE_ITEM ODSCRIPTS od_external.bat )
endif()

if( ${OD_PLFSUBDIR} STREQUAL "lux64" )
    set( SYSTEMLIBS libstdc++.so.6 libgcc_s.so.1 )
endif()

set ( PYTHONDIR python )

set( PACK "base" )

