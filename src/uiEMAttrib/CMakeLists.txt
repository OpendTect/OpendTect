#_______________________Pmake___________________________________________________
#
#	Makefile : 	uiEMAttrib
# 	October 2004	N. Hemstra
#_______________________________________________________________________________

set(OD_MODULE_DEPS uiAttributes uiEarthModel EMAttrib )
set(OD_MODULE_SOURCES
	uiattremout.cc
	uiattrsurfout.cc
	uiattrtrcselout.cc
	uicreate2dgrid.cc
	uiemattribpartserv.cc
	uihor2dfrom3ddlg.cc
	uihorizonshiftdlg.cc
	horizon2dscanner.cc
	uiimphorizon2d.cc
	uiseiseventsnapper.cc
)
OD_INIT_MODULE()
