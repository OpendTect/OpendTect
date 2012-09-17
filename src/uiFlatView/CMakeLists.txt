#_______________________Pmake___________________________________________________
#
#	Makefile : 	uiFlatView
# 	Feb 2007	Bert
#	RCS :		$Id: CMakeLists.txt,v 1.2 2012/02/16 20:04:40 cvskris Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiTools )
SET(OD_MODULE_SOURCES
	flatviewbitmap.cc flatviewzoommgr.cc flatviewaxesdrawer.cc
	uiflatauxdataeditor.cc uiflatviewer.cc uiflatviewwin.cc
	uiflatviewcontrol.cc uiflatviewpropdlg.cc uiflatviewstdcontrol.cc
	uiflatauxdataeditorlist.cc uiflatviewthumbnail.cc
	uiflatviewcoltabed.cc uiflatviewslicepos.cc
	uimultiflatviewcontrol.cc
)

OD_INIT_MODULE()
