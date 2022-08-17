#________________________________________________________________________
#
# Copyright:    dGB Beheer B.V.
# License:      https://dgbes.com/index.php/licensing
#________________________________________________________________________
#

set( OD_MODULE_DEPS uiWell uiAttributes WellAttrib )
set( OD_MODULE_SOURCES
	inituiwellattrib.cc
	uicreateattriblogdlg.cc
	uicreatelogcubedlg.cc
	uiodstratlayermodelmgr.cc
	uistratlayermodel.cc
	uistratseisevent.cc
	uistratsynthcrossplot.cc
	uistratsynthdisp.cc
	uistratsynthexport.cc
	uisynthgendlg.cc
	uisynthseis.cc
	uisynthtorealscale.cc
	uiwellattribpartserv.cc
	uiwellattribxplot.cc
	uiwelllogattrib.cc
	uiwelllogextract.cc
	uiwelltiecheckshotedit.cc
	uiwelltiecontrolview.cc
	uiwelltieeventstretch.cc
	uiwelltiemgrdlg.cc
	uiwelltiesavedatadlg.cc
	uiwelltietoseismicdlg.cc
	uiwelltieview.cc
	uiwelltiewavelet.cc
	uiwellto2dlinedlg.cc
)

OD_INIT_MODULE()
