#_______________________Pmake___________________________________________________
#
#	Makefile : 	uiWellAttrib
# 	Jul 2000	A.H. Bril
#_______________________________________________________________________________

set( OD_MODULE_DEPS uiWell uiAttributes WellAttrib )
set( OD_MODULE_SOURCES
	uicreateattriblogdlg.cc
	uicreatelogcubedlg.cc
	uiwellattribpartserv.cc
	uiwellattribxplot.cc
	uiwelllogattrib.cc
	uiwelllogextract.cc
	uiwelltiecontrolview.cc
	uiwelltiecheckshotedit.cc
	uiwelltieeventstretch.cc
	uiwelltietoseismicdlg.cc
	uiwelltiemgrdlg.cc
	uiwelltiesavedatadlg.cc
	uiwelltieview.cc
	uiwelltiewavelet.cc
	uiwellto2dlinedlg.cc
	uistratlayermodel.cc
	uistratseisevent.cc
	uistratsynthcrossplot.cc
	uistratsynthdisp.cc
	uistratsynthexport.cc
	uisynthtorealscale.cc
	uisynthgendlg.cc
	uisynthseis.cc
	inituiwellattrib.cc
)

OD_INIT_MODULE()
