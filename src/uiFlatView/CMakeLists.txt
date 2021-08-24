#_______________________Pmake___________________________________________________
#
#	Makefile : 	uiFlatView
# 	Feb 2007	Bert
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiTools )
SET(OD_MODULE_SOURCES
	flatviewzoommgr.cc
	flatviewaxesdrawer.cc
	uiflatauxdatadisplay.cc
	uiflatauxdataeditor.cc
	uiflatauxdataeditorlist.cc
	uibitmapdisplay.cc
	uiflatviewcoltabed.cc
	uiflatviewcontrol.cc
	uiflatviewer.cc
	uiflatviewpropdlg.cc
	uiflatviewslicepos.cc
	uiflatviewstdcontrol.cc
	uiflatviewwin.cc
	uimultiflatviewcontrol.cc
)

OD_INIT_MODULE()
