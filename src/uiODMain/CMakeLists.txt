#________________________________________________________________________
#
# Copyright:    dGB Beheer B.V.
# License:      https://dgbes.com/index.php/licensing
#________________________________________________________________________
#

set(OD_MODULE_DEPS
	uiVis
	uiVolumeProcessing
	uiNLA
	uiWellAttrib
	uiEMAttrib
	uiViewer2D
)
set(OD_MODULE_SOURCES
	inituiodmain.cc
	measuretoolman.cc
	odsession.cc
	uiodannottreeitem.cc
	uiodannotarrowtreeitem.cc
	uiodannotimagetreeitem.cc
	uiodannotscalebaritem.cc
	uiodapplmgr.cc
	uiodapplmgraux.cc
	uiodapplmgrattrvis.cc
	uiodattribtreeitem.cc
	uiodbodydisplaytreeitem.cc
	uiodcontourtreeitem.cc
	uioddatatreeitem.cc
	uioddisplaytreeitem.cc
	uiodfaulttoolman.cc
	uiodfaulttreeitem.cc
	uiodeditattribcolordlg.cc
	uiodhelpmenumgr.cc
	uiodmain.cc
	uiodmenumgr.cc
	uiodscenemgr.cc
	uiodtreeitem.cc
	uiodemsurftreeitem.cc
	uiodhortreeitem.cc
	uiodpicksettreeitem.cc
	uiodplanedatatreeitem.cc
	uiodpseventstreeitem.cc
	uiodrandlinetreeitem.cc
	uiodseis2dtreeitem.cc
	uiodviewer2d.cc
	uiodviewer2dmgr.cc
	uiodviewer2dposgrp.cc
	uiodviewer2dposdlg.cc
	uiodvw2dtreeitem.cc
	uiodvw2dfaulttreeitem.cc
	uiodvw2dfaultsstreeitem.cc
	uiodvw2dfaultss2dtreeitem.cc
	uiodvw2dhor2dtreeitem.cc
	uiodvw2dhor3dtreeitem.cc
	uiodvw2dpicksettreeitem.cc
	uiodvw2dwigglevararea.cc
	uiodvw2dvariabledensity.cc
	uiodvolrentreeitem.cc
	uiodvolproctreeitem.cc
	uiodwelltreeitem.cc
	uiodlangmenumgr.cc
	uifirewallprocsetterdlg.cc
	uiserviceclientmgr.cc
)

set( OD_MODULE_GUI_PROGS od_main.cc  )
if ( WIN32 )
    set( OD_MODULE_PROGS od_main_console.cc )
    set( OD_od_main_RESOURCE resources/icon1.ico )
    list( APPEND OD_ELEVATED_PERMISSIONS_GUI_PROGS od_FirewallProcSetter.cc )
endif()

OD_INIT_MODULE()
