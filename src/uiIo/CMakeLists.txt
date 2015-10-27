#_______________________Pmake___________________________________________________
#
#	Makefile : 	uiIo - OpendTect specific tools on top of uiBase/Tools
# 	Jul 2000	A.H. Bril
#	RCS :		$Id$
#_______________________________________________________________________________

add_definitions( -DmNoAutoUiString )

set( OD_MODULE_DEPS uiTools MMProc uiFlatView Geometry )
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
	uigraphicslegend.cc
	uigraphicsscalebar.cc
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
	uilatlong.cc
	uilistboxchoiceio.cc
	uimanprops.cc
	uimathformula.cc
	uimathpropeddlg.cc
	uimmbatchjobdispatch.cc
	uiobjfileman.cc
	uipickpartserv.cc
	uipicksetman.cc
	uipicksetmgr.cc
	uiposfilterset.cc
	uiposfiltgroup.cc
	uiposprovider.cc
	uiposprovgroup.cc
	uiprobdenfuncgen.cc
	uiprobdenfuncman.cc
	uiprobdenfunvarsel.cc
	uirandomlineman.cc
	uisetdatadir.cc
	uiselobjothersurv.cc
	uiselsurvranges.cc
	uisrchprocfiles.cc
	uisurvey.cc
	uisurveyselect.cc
	uisurveyzip.cc
	uisurvinfoed.cc
	uisurvmap.cc
	uit2dconvsel.cc
	inituiio.cc
)

if( WIN32 )
    set( OD_ELEVATED_PERMISSIONS_GUI_PROGS od_BatchHosts.cc )
    set( OD_od_BatchHosts_RESOURCE  resources/od_BatchHosts.rc )
else()
    set( OD_MODULE_GUI_PROGS od_BatchHosts.cc )
endif()

OD_INIT_MODULE()
