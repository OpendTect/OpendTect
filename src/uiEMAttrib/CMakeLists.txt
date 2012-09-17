#_______________________Pmake___________________________________________________
#
#	Makefile : 	uiEMAttrib
# 	October 2004	N. Hemstra
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012/01/23 20:55:23 cvskris Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiAttributes uiEarthModel EMAttrib )
SET(OD_MODULE_SOURCES
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
		uiimpfaultstickset2d.cc
)
OD_INIT_MODULE()
