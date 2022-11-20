#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( OD_MODULE_DEPS uiTools uiFlatView Geometry )
set( OD_FOLDER "Base (GUI)" )

set( OD_MODULE_SOURCES
	dpsdensitycalc.cc
	ui2dgeomman.cc
	ui2dsip.cc
	uibatchhostsdlg.cc
	uibatchlaunch.cc
	uibatchprogs.cc
	uiclusterjobprov.cc
	uiconvpos.cc
	uicrdevenv.cc
	uicreatedpspdf.cc
	uicreatepicks.cc
	uidatapointset.cc
	uidatapointsetcrossplot.cc
	uidatapointsetman.cc
	uidatapointsetmerger.cc
	uidatapointsetcrossplotwin.cc
	uidpsaddcolumndlg.cc
	uidpscrossplotpropdlg.cc
	uidpscrossplottools.cc
	uidpsoverlayattrdlg.cc
	uidpsrefineseldlg.cc
	uidpsselgrpdlg.cc
	uidpsselectednessdlg.cc
	uieditpdf.cc
	uigeom2dsel.cc
	uigraphicslegend.cc
	uiimppickset.cc
	uiimpexpselgrp.cc
	uiimpexppdf.cc
	uiimpexp2dgeom.cc
	uiimppvds.cc
	uiioobj.cc
	uiioobjmanip.cc
	uiioobjsel.cc
	uiioobjselgrp.cc
	uiioobjselwritetransl.cc
	uilistboxchoiceio.cc
	uimanprops.cc
	uimathformula.cc
	uimathpropeddlg.cc
	uimmbatchjobdispatch.cc
	uiobjfileman.cc
	uipickpartserv.cc
	uipicksetman.cc
	uipicksetmgr.cc
	uipointsetsel.cc
	uiposfilterset.cc
	uiposfiltgroup.cc
	uiposprovider.cc
	uiposprovgroup.cc
	uiprobdenfuncgen.cc
	uiprobdenfuncman.cc
	uiprobdenfunvarsel.cc
	uirandomlineman.cc
	uiselobjothersurv.cc
	uiselsurvranges.cc
	uisimpletimedepthmodel.cc
	uisrchprocfiles.cc
	uisurvey.cc
	uisurveyfile.cc
	uisurveyselect.cc
	uisurvinfoed.cc
	uisurvmap.cc
	uit2dconvsel.cc
	inituiio.cc
)

set( OD_MODULE_GUI_PROGS
	od_BatchHosts.cc
	od_RemoteServiceMgr.cc
)

if( WIN32 )
    set( RC_od_BatchHosts_DESC "Set up Distributed Computing" )
endif()

OD_INIT_MODULE()
