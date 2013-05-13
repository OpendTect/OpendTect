#_______________________Pmake___________________________________________________
#
#	Makefile: 	uiODMain
# 	Nov 2005:	Nanne Hemstra
#	RCS :		$Id$
#_______________________________________________________________________________

set(OD_MODULE_DEPS
	uiVis
	uiAttributes
	uiNLA
	uiWellAttrib
	uiEMAttrib 
	uiViewer2D
)
set(OD_MODULE_SOURCES
	inituiodmain.cc
	odsession.cc
	uiodapplmgr.cc
	uiodapplmgraux.cc
	uiodapplmgrattrvis.cc
	uiodattribtreeitem.cc
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
	uiodbodydisplaytreeitem.cc
	uiodseis2dtreeitem.cc
	uiodviewer2d.cc
	uiodviewer2dmgr.cc
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
)

set( OD_MODULE_GUI_PROGS od_main.cc  )
if(WIN32)
    set(OD_MODULE_GUI_PROGS ${OD_MODULE_GUI_PROGS} od_start_dtect.cc  )
    set(OD_MODULE_PROGS od_main_console.cc )
    set(OD_od_main_RESOURCE  resources/od_main.rc )
    set(OD_od_start_dtect_RESOURCE resources/od_main.rc )
endif()

OD_ADD_SOURCE_FILES( resources/od_main.rc resources/resource.h resources/icon1.ico scene.xpm )

OD_INIT_MODULE()
