#_______________________Pmake___________________________________________________
#
#	Makefile : 	uiIo - OpendTect specific tools on top of uiBase/Tools
# 	Jul 2000	A.H. Bril
#_______________________________________________________________________________

set( OD_MODULE_DEPS uiTools MMProc uiFlatView Geometry )
set( OD_MODULE_SOURCES
	dpsdensitycalc.cc
	ui2dgeomman.cc
	ui2dsip.cc
	uiautosaverdlg.cc
	uibatchhostsdlg.cc
	uibatchlaunch.cc
	uibatchprogs.cc
	uiclusterjobprov.cc
	uiconvpos.cc
	uicoordsystem.cc
	uicrdevenv.cc
	uicreatedpspdf.cc
	uicreatepicks.cc
	uidatapointset.cc
	uidatapointsetcrossplot.cc
	uidatapointsetman.cc
	uidatapointsetmerger.cc
	uidatapointsetcrossplotwin.cc
	uidatarootsel.cc
	uidpsaddcolumndlg.cc
	uidpscrossplotpropdlg.cc
	uidpscrossplottools.cc
	uidpsoverlayattrdlg.cc
	uidpsrefineseldlg.cc
	uidpsselgrpdlg.cc
	uidpsselectednessdlg.cc
	uieditpdf.cc
	uifixinvaliddataroot.cc
	uigraphicslegend.cc
	uigraphicsscalebar.cc
	uiimppickset.cc
	uiimpexpselgrp.cc
	uiimpexppdf.cc
	uiimpexp2dgeom.cc
	uiimppvds.cc
	uiiocommon.cc
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
	uipicksettools.cc
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
	uisrchprocfiles.cc
	uisurveymanager.cc
	uisurveyselect.cc
	uisurvinfoed.cc
	uisurvmap.cc
	uiusercreatesurvey.cc
	uit2dconvsel.cc
	inituiio.cc
)

set( OD_DATAROOT_UI_PROGRAMS od_Edit_Survey.cc od_ManageSurveys.cc od_SelectObject.cc )

if( WIN32 )
    set( OD_ELEVATED_PERMISSIONS_GUI_PROGS od_BatchHosts.cc )
    set( OD_od_BatchHosts_RESOURCE resources/od_BatchHosts.rc )
    set( OD_MODULE_GUI_PROGS ${OD_DATAROOT_UI_PROGRAMS} )
else()
    set( OD_MODULE_GUI_PROGS od_BatchHosts.cc ${OD_DATAROOT_UI_PROGRAMS} )
endif()

OD_INIT_MODULE()
