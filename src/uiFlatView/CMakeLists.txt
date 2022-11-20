#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( OD_MODULE_DEPS uiTools )
set( OD_FOLDER "Base (GUI)" )

set( OD_MODULE_SOURCES
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
