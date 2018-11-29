#_______________________Pmake___________________________________________________
#
#	Makefile : 	uiIo - OpendTect specific tools on top of uiBase/Tools
# 	Jul 2000	Bert
#_______________________________________________________________________________

set( OD_MODULE_DEPS uiTools uiFlatView Geometry )
set( OD_MODULE_SOURCES
	dpsdensitycalc.cc
	ui2dgeomman.cc
	uiautosavesettings.cc
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
	uifixinvaliddataroot.cc
	uigeom2dsel.cc
	uigraphicslegend.cc
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
	uiselsurvranges.cc
	uisrchprocfiles.cc
	uisipimpl.cc
	uistorcollbuilder.cc
	uisurveymanager.cc
	uisurveyselect.cc
	uisurvinfoed.cc
	uisurvioobjsel.cc
	uisurvmap.cc
	uiusercreatesurvey.cc
	uit2dconvsel.cc
	inituiio.cc
)


if( WIN32 )
    set( OD_od_BatchHosts_RESOURCE resources/od_BatchHosts.rc )
endif()

set( OD_DATAROOT_UI_PROGRAMS od_Edit_Survey.cc od_Manage_Surveys.cc )
set( OD_MODULE_GUI_PROGS od_BatchHosts.cc ${OD_DATAROOT_UI_PROGRAMS} )

OD_INIT_MODULE()
