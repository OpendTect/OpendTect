#_______________________Pmake___________________________________________________
#
#	Makefile : 	ui- based on vis stuff.
# 	July 2002	Nanne Hemstra
#_______________________________________________________________________________

set(OD_MODULE_DEPS uiOSG uiMPE )
set(OD_MODULE_SOURCES
	inituivis.cc
	uiarrowdlg.cc
	uiattribtransdlg.cc
	uidatapointsetpickdlg.cc
	uifltdispoptgrp.cc
	uigridlinesdlg.cc
	uimarkerstyledlg.cc
	uimaterialdlg.cc
	uimpeman.cc
	uipickpropdlg.cc
	uiscenecolorbarmgr.cc
	uiscenepropdlg.cc
	uiseedpropdlg.cc
	uisurvtopbotimg.cc
	uiviscoltabed.cc
	uivisdatapointsetdisplaymgr.cc
	uivisemobj.cc
	uivisisosurface.cc
	uivismenuitemhandler.cc
	uivispartserv.cc
	uivispickretriever.cc
	uivisplanedatadisplaydragprop.cc
	uivispolygonsurfbezierdlg.cc
	uivisslicepos3d.cc
	uiviszstretchdlg.cc
	uivisdirlightdlg.cc
)

OD_INIT_MODULE()
