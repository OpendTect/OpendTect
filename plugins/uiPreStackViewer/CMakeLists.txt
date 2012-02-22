#_______________________Pmake___________________________________________________
#
#	Makefile : 	Tutorial plugin
# 	Jan 2012	A.H. Bril
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012-02-22 09:44:59 cvskris Exp $
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
SET( OD_PLUGIN_EXECS od_main )
OD_INIT_MODULE()
