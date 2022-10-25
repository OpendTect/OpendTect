#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( OD_MODULE_DEPS uiBase MMProc )
set( OD_MODULE_SOURCES
	inituitools.cc
	annotbuffill.cc
	uiamplspectrum.cc
	uiapplserv.cc
	uiarray1dinterpol.cc
	uiarray2dchg.cc
	uiarray2dinterpol.cc
	uiaxisdata.cc
	uiaxishandler.cc
	uibasemap.cc
	uibatchjob.cc
	uibinidtable.cc
	uibuildlistfromlist.cc
	uibuttonstateedit.cc
	uichecklist.cc
	uicolortable.cc
	uicoltabimport.cc
	uicoltabman.cc
	uicoltabmarker.cc
	uicoltabtools.cc
	uicompparsel.cc
	uiconstvel.cc
	uicoordsystem.cc
	uidatapackmon.cc
	uidatarootsel.cc
	uidirectionalplot.cc
	uidlggroup.cc
	uifileinput.cc
	uifillpattern.cc
	uifontsel.cc
	uifontselgrp.cc
	uifreqfilter.cc
	uifuncdispbase.cc
	uifuncdrawerbase.cc
	uifunctiondisplay.cc
	uifunctiondisplayserver.cc
	uigeninput.cc
	uigeninput_impl.cc
	uigeninputdlg.cc
	uigraphicssaveimagedlg.cc
	uigraphicsscalebar.cc
	uigraphicssceneaxismgr.cc
	uigraphicsview.cc
	uigridder2d.cc
	uigvfreehandareaselectiontool.cc
	uihistogramdisplay.cc
	uihistogramsel.cc
	uihostiddlg.cc
	uihostinpgrp.cc
	uiinterpollayermodel.cc
	uiiosel.cc
	uiissuereporter.cc
	uilatlong.cc
	uilistboxfilter.cc
	uilocalhostgrp.cc
	uimapperrangeeditor.cc
	uimapperrangeeditordlg.cc
	uimarkerstyle.cc
	uimathexpression.cc
	uimathexpressionvariable.cc
	uimeasuredlg.cc
	uimenuhandler.cc
	uimnemonicsel.cc
	uinotsaveddlg.cc
	uiobjectitemview.cc
	uiobjectitemviewwin.cc
	uiofferinfo.cc
	uipathsel.cc
	uipluginman.cc
	uipolardiagram.cc
	uipositiontable.cc
	uipropvalfld.cc
	uiproxydlg.cc
	uiraytrace1d.cc
	uirefltrace1d.cc
	uirgbarraycanvas.cc
	uirockphysform.cc
	uisaveimagedlg.cc
	uiscaler.cc
	uisellinest.cc
	uiselsimple.cc
	uiserviceservermgr.cc
	uisetdatadir.cc
	uisettings.cc
	uishortcuts.cc
	uisizesel.cc
	uislicepos.cc
	uislicesel.cc
	uistatsdisplay.cc
	uistatusbutton.cc
	uistepoutsel.cc
	uistratlvlsel.cc
	uisurveyzip.cc
	uitaskrunner.cc
	uitblimpexpdatasel.cc
	uitextfile.cc
	uitime2depthzaxistrans.cc
	uitoolbarcmded.cc
	uitreeitemmanager.cc
	uiunitsel.cc
	uivalseriestracker.cc
	uivariogram.cc
	uivarwizard.cc
	uiwindowfuncseldlg.cc
	uiwindowfunctionsel.cc
	uiwindowgrabber.cc
	uiwizard.cc
	uiworld2ui.cc
	uizaxistransform.cc
	uizrangeinput.cc
)

set( OD_MODULE_GUI_PROGS
	od_FileBrowser.cc
	od_ImageViewer.cc
	od_ProgressViewer.cc
	od_uiReportIssue.cc
)

if( WIN32 )
    list( APPEND OD_ELEVATED_PERMISSIONS_GUI_PROGS od_runinst.cc )
    set( RC_od_runinst_DESC "OpendTect Installer" )
endif( WIN32 )

if ( OD_ENABLE_BREAKPAD )
    list ( APPEND OD_MODULE_TESTPROGS test_breakpad.cc )
endif()


OD_INIT_MODULE()
