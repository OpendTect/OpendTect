#_______________________Pmake___________________________________________________
#
#	Makefile : 	EarthModel
# 	May 2001 :	Bert
#	RCS :		$Id: CMakeLists.txt 7975 2013-06-10 04:57:00Z ranojay.sen@dgbes.com $
#_______________________________________________________________________________

SET( OD_MODULE_DEPS uiCmdDriver )
SET( OD_MODULE_SOURCES
	odinstappdata.cc
	odinstdlhandler.cc
	odinsthtmlcomposer.cc
	odinstinstallhandler.cc
	odinstlogger.cc
	odinstpkgkey.cc
	odinstpkgprops.cc
	odinstpkgselmgr.cc
	odinstrel.cc
	odinstziphandler.cc
	uiodinstdlg.cc
	uiodinstmgr.cc
	uiodinstpkgmgr.cc
)

IF ( WIN32 )
   SET ( OD_SET_LINKFLAGS_UAC 1 )
   LIST( APPEND OD_MODULE_SOURCES odinstwinutils.cc )
   SET( OD_MODULE_GUI_PROGS od_runinst.cc od_setup.cc )
ENDIF( WIN32 )

SET( OD_MODULE_GUI_PROGS ${OD_MODULE_GUI_PROGS} od_instmgr.cc )

IF(WIN32)
    SET(OD_od_instmgr_RESOURCE od_instmgr.rc )
    SET(OD_od_runinst_RESOURCE od_instmgr.rc )
    SET(OD_od_setup_RESOURCE od_instmgr.rc )
ENDIF()
OD_INIT_MODULE()
