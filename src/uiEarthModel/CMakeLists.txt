#_______________________Pmake___________________________________________________
#
#	Makefile: 	ui- based on EarthModel
# 	May 2002	Nanne Hemstra
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiStrat EarthModel )
SET(OD_MODULE_SOURCES
	inituiearthmodel.cc
	uibodyposprovgroup.cc
	uibodyregiondlg.cc
	uibodyoperatordlg.cc
	uibulkfaultimp.cc
	uibulkhorizonimp.cc
	uibulk2dhorizonimp.cc
	uicalcpoly2horvol.cc
	uichangesurfacedlg.cc
	uicreatehorizon.cc
	uiemauxdatasel.cc
	uiemhorizonpreloaddlg.cc
	uiempartserv.cc
	uiexpfault.cc
	uiexphorizon.cc
	uiexport2dhorizon.cc
	uifaultsetcopy.cc
	uihorauxdatasel.cc
	uihorinterpol.cc
	uihor3dfrom2ddlg.cc
	uihorgeom2attr.cc
	uihorizonmergedlg.cc
	uihorizonrelations.cc
	uihorizonsortdlg.cc
	uihorsavefieldgrp.cc
	uiimpbodycaldlg.cc
	uiimpfault.cc
	uiimphorizon.cc
	uiiosurface.cc
	uiiosurfacedlg.cc
	uiisopachmaker.cc
	uimultisurfaceread.cc
	uipolygonzchanger.cc
	uirandlinegen.cc
	uisurfaceman.cc
	uisurfaceposprov.cc
	uisurfacesel.cc
)
OD_INIT_MODULE()
