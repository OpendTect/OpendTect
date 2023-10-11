#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( OD_MODULE_DEPS uiAttributes uiEarthModel EMAttrib )
set( OD_FOLDER "Base (GUI)" )

set( OD_MODULE_SOURCES
	uiattremout.cc
	uiattrsurfout.cc
	uiattrtrcselout.cc
	uicreate2dgrid.cc
	uiemattribpartserv.cc
	uiemattrsel.cc
	uiflatunflatcube.cc
	uiflattenedcube.cc
	uihor2dfrom3ddlg.cc
	uihorizonattrib.cc
	uihorizonshiftdlg.cc
	uiimphorizon2d.cc
	uiseiseventsnapper.cc
	uistratamp.cc
	horizon2dscanner.cc
	inituiemattrib.cc
)

OD_INIT_MODULE()
