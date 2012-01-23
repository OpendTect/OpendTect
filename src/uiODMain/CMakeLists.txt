#_______________________Pmake___________________________________________________
#
#	Makefile: 	uiODMain
# 	Nov 2005:	Nanne Hemstra
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012-01-23 20:55:23 cvskris Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiVis )
SET(OD_MODULE_SOURCES
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

SET(OD_MODULE_EXECS od_main )

OD_INIT_MODULE()
