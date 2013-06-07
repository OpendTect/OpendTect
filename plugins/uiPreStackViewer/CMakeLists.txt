#_______________________Pmake___________________________________________________
#
#	Makefile : 	Tutorial plugin
# 	Jan 2012	A.H. Bril
#	RCS :		$Id: CMakeLists.txt,v 1.3 2012/03/29 11:33:26 cvskris Exp $
#_______________________________________________________________________________

SET( OD_MODULE_DEPS uiODMain )
SET( OD_IS_PLUGIN yes)
SET( OD_MODULE_SOURCES
		uipsviewerpi.cc
		psviewer2dgatherpainter.cc
		uipseventstreeitem.cc
		uiprestacktreeitemmgr.cc
		uipsviewerappearancetab.cc
		uipsviewermanager.cc
		uipsviewerposdlg.cc
		uipsviewerpreproctab.cc
		uipsviewerscalingtab.cc
		uipsviewersettingdlg.cc
		uipsviewershapetab.cc
		uipsviewer2d.cc
		uipsviewer2dinfo.cc
		uipsviewer2dmainwin.cc
		uipsviewer2dposdlg.cc
		visprestackviewer.cc
		vispseventdisplay.cc )
SET( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )
OD_INIT_MODULE()
