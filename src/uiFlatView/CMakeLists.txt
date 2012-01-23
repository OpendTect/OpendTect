#_______________________Pmake___________________________________________________
#
#	Makefile : 	uiFlatView
# 	Feb 2007	Bert
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012-01-23 20:55:23 cvskris Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiTools )
SET(OD_MODULE_SOURCES
	flatviewbitmap.cc flatviewzoommgr.cc flatviewaxesdrawer.cc
	uiflatauxdataeditor.cc uiflatviewer.cc uiflatviewwin.cc
	uiflatviewcontrol.cc uiflatviewpropdlg.cc uiflatviewstdcontrol.cc
	uiflatauxdataeditorlist.cc uiflatviewthumbnail.cc
	uiflatviewcoltabed.cc uiflatviewslicepos.cc
)

OD_INIT_MODULE()
