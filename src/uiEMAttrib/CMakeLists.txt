#_______________________Pmake___________________________________________________
#
#	Makefile : 	uiEMAttrib
# 	October 2004	N. Hemstra
#	RCS :		$Id$
#_______________________________________________________________________________

add_definitions( -DmNoAutoUiString )

set(OD_MODULE_DEPS uiAttributes uiEarthModel EMAttrib )
set(OD_MODULE_SOURCES
	uiattremout.cc
	uiattrsurfout.cc
	uiattrtrcselout.cc
	uicreate2dgrid.cc
	uiemattribpartserv.cc
	uiflattenedcube.cc
	uihor2dfrom3ddlg.cc
	uihorizonshiftdlg.cc
	horizon2dscanner.cc
	uiimphorizon2d.cc
	uiseiseventsnapper.cc
)
OD_INIT_MODULE()

