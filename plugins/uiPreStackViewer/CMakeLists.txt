#_______________________Pmake___________________________________________________
#
#	Makefile : 	PreStack Viewer
# 	Jan 2012	Y.C. Liu
#_______________________________________________________________________________

set( OD_MODULE_DEPS uiODMain )
set( OD_IS_PLUGIN yes)
set( OD_MODULE_SOURCES
		uipsviewerpi.cc
		uipsviewerappearancetab.cc
		uipsviewermanager.cc
		uipsviewerposdlg.cc
		uipsviewerpreproctab.cc
		uipsviewerscalingtab.cc
		uipsviewersettingdlg.cc
		uipsviewershapetab.cc
)
set( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )
OD_INIT_MODULE()
