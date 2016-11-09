#_______________________Pmake___________________________________________________
#
#	Makefile : 	SEG-Y UI
# 	June 2015	Bert
#	RCS :		$Id$
#_______________________________________________________________________________

set(OD_MODULE_DEPS uiODMain)
set(OD_IS_PLUGIN yes)
set(OD_MODULE_SOURCES
    segyhdrkeydata.cc
    segyuiscandata.cc
    uisegycommon.cc
    uisegydef.cc
    uisegydefdlg.cc
    uisegydirectinserter.cc
    uisegyexamine.cc
    uisegyexp.cc
    uisegyimpdlg.cc
    uisegyimpparsdlg.cc
    uisegyimptype.cc
    uisegymanip.cc
    uisegypi.cc
    uisegyread.cc
    uisegyreaddlg.cc
    uisegyreadfinisher.cc
    uisegyresortdlg.cc
    uisegyscandlg.cc
    uisegyreadstarter.cc
    uisegyreadstartinfo.cc
    uisegysip.cc
    uisegysipclassic.cc
    uisegytrchdrvalplot.cc
    uisegywriteopts.cc
    uiwellimpsegyvsp.cc
)

set( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )
set( OD_MODULE_PROGS od_SEGYExaminer.cc )
OD_INIT_MODULE()
