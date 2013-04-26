#_______________________Pmake___________________________________________________
#
#	Makefile : 	uiWellAttrib
# 	Jul 2000	A.H. Bril
#	RCS :		$Id$
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiWell uiAttributes WellAttrib )
SET(OD_MODULE_SOURCES
		uicreateattriblogdlg.cc
		uicreatelogcubedlg.cc
		uiwellattribpartserv.cc
		uiwellattribxplot.cc
		uiwellimpsegyvsp.cc
		uiwelllogextract.cc
		uiwelltiecontrolview.cc
		uiwelltiecheckshotedit.cc
		uiwelltieeventstretch.cc
		uiwelltietoseismicdlg.cc
		uiwelltiemgrdlg.cc
		uiwelltiesavedatadlg.cc
		uiwelltieview.cc
		uiwelltiewavelet.cc
		uistratlayermodel.cc
		uistratseisevent.cc
		uistratsynthcrossplot.cc
		uistratsynthdisp.cc
		uisynthtorealscale.cc
		uisynthgendlg.cc
		inituiwellattrib.cc
)

OD_INIT_MODULE()
