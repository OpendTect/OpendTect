#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( OD_MODULE_DEPS uiOSG uiMPE )
set( OD_FOLDER "Visualization (3D)" )

set( OD_MODULE_SOURCES
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
	uivisdirlightdlg.cc
	uivisemobj.cc
	uivisisosurface.cc
	uivismenuitemhandler.cc
	uivispartserv.cc
	uivispickretriever.cc
	uivisplanedatadisplaydragprop.cc
	uivispolygonsurfbezierdlg.cc
	uivisslicepos3d.cc
	uiviszstretchdlg.cc
)

OD_INIT_MODULE()
