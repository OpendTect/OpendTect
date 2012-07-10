#_______________________Pmake___________________________________________________
#
#	Makefile : 	uiFlatView
# 	Feb 2007	Bert
#	RCS :		$Id: CMakeLists.txt,v 1.3 2012-07-10 13:27:27 cvsbruno Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiTools )
SET(OD_MODULE_SOURCES
	flatviewbitmap.cc flatviewzoommgr.cc flatviewaxesdrawer.cc
	uiflatauxdataeditor.cc uiflatviewer.cc uiflatviewwin.cc
	uiflatviewcontrol.cc uiflatviewpropdlg.cc uiflatviewstdcontrol.cc
	uiflatauxdataeditorlist.cc uiflatviewthumbnail.cc
	uiflatviewcoltabed.cc uiflatviewslicepos.cc
	uimultiflatviewcontrol.cc uiflatbitmapdisplay.cc
	uiflatauxdatadisplay.cc
)

OD_INIT_MODULE()
