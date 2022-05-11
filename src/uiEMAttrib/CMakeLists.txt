#_______________________Pmake___________________________________________________
#
#	Makefile : 	uiEMAttrib
# 	October 2004	N. Hemstra
#_______________________________________________________________________________

set(OD_MODULE_DEPS uiAttributes uiEarthModel EMAttrib )
set(OD_MODULE_SOURCES
	horizon2dscanner.cc
	uiattremout.cc
	uiattrsurfout.cc
	uiattrtrcselout.cc
	uicreate2dgrid.cc
	uiemattribpartserv.cc
	uiflatunflatcube.cc
	uihor2dfrom3ddlg.cc
	uihorizonshiftdlg.cc
	uiimphorizon2d.cc
	uiseiseventsnapper.cc
)
OD_INIT_MODULE()
