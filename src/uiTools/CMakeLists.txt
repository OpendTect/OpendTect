#_______________________Pmake___________________________________________________
#
#	Makefile : 	General uiToolkits
# 	Jan 2012	K. Tingdahl
#_______________________________________________________________________________

set( OD_MODULE_DEPS uiBase MMProc )
set( OD_MODULE_SOURCES
	inituitools.cc
	uiamplspectrum.cc
	uiapplserv.cc
	uiarray2dchg.cc
	uiarrayndinterpol.cc
	uiaxisdata.cc
	uiaxishandler.cc
	uibasemap.cc
	uibatchjob.cc
	uibinidtable.cc
	uibuildlistfromlist.cc
	uibuttonstateedit.cc
	uichecklist.cc
	uicolseqdisp.cc
	uicolseqman.cc
	uicolseqsel.cc
	uicolseqimport.cc
	uicoltabsel.cc
	uicoltabgraphicsitem.cc
	uicompparsel.cc
	uiconstvel.cc
	uicoordsystem.cc
	uidatarootsel.cc
	uidirectionalplot.cc
	uidlggroup.cc
	uifileinput.cc
	uifilesel.cc
	uifillpattern.cc
	uifontsel.cc
	uifreqfilter.cc
	uifunctiondisplay.cc
	uigeninput.cc
	uigeninput_impl.cc
	uigeninputdlg.cc
	uigraphicsscalebar.cc
	uigraphicssaveimagedlg.cc
	uigraphicssceneaxismgr.cc
	uigraphicsview.cc
	uigridder2d.cc
	uigvfreehandareaselectiontool.cc
	uihandledlsitefail.cc
	uihistogramdisplay.cc
	uihistogramsel.cc
	uiiconsetsel.cc
	uiinterpollayermodel.cc
	uiiosel.cc
	uiissuereporter.cc
	uilanguagesel.cc
	uilistboxfilter.cc
	uimapperrangeeditor.cc
	uimenuhandler.cc
	uimapperrangeeditordlg.cc
	uimarkerstyle.cc
	uimathexpression.cc
	uimathexpressionvariable.cc
	uimeasuredlg.cc
	uinotsaveddlg.cc
	uiobjectitemview.cc
	uiobjectitemviewwin.cc
	uiodtreeitem.cc
	uiodprmantreeitem.cc
	uiofferinfo.cc
	uipathsel.cc
	uipluginman.cc
	uipluginsel.cc
	uipositiontable.cc
	uipropvalfld.cc
	uiproxydlg.cc
	uiserviceservermgr.cc
	uiraytrace1d.cc
	uirgbarraycanvas.cc
	uirockphysform.cc
	uisaveimagedlg.cc
	uiscaler.cc
	uisellinest.cc
	uiselsimple.cc
	uisettings.cc
	uislicepos.cc
	uislicesel.cc
	uistatsdisplay.cc
	uistepoutsel.cc
	uistratlvlsel.cc
	uitblimpexpdatasel.cc
	uitaskrunner.cc
	uitextfile.cc
	uithemesel.cc
	uitime2depthzaxistrans.cc
	uitoolbarcmded.cc
	uitreeitem.cc
	uiunitsel.cc
	uivalseriestracker.cc
	uivariogram.cc
	uivarwizard.cc
	uiwindowfunctionsel.cc
	uiwindowfuncseldlg.cc
	uiwindowgrabber.cc
	uiworld2ui.cc
	uizaxistransform.cc
	uizrangeinput.cc
	uipolardiagram.cc
	uicrssystem.cc
)
if ( WIN32 )
	list( APPEND OD_MODULE_SOURCES uifirewallprocsetterdlg.cc )
endif()

set( OD_MODULE_GUI_PROGS
	od_FileBrowser.cc
	od_ImageViewer.cc
	od_ProgressViewer.cc
	od_uiReportIssue.cc
)

if( WIN32 )
    list( APPEND OD_ELEVATED_PERMISSIONS_GUI_PROGS od_runinst.cc )
    set( OD_od_runinst_RESOURCE resources/od_runinst.rc )
endif( WIN32 )

if ( OD_ENABLE_BREAKPAD )
    list ( APPEND OD_MODULE_TESTPROGS test_breakpad.cc )
endif()


OD_INIT_MODULE()
