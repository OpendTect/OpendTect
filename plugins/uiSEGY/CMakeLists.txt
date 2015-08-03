#_______________________Pmake___________________________________________________
#
#	Makefile : 	SEG-Y UI
# 	June 2015	Bert
#	RCS :		$Id$
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiODMain)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
    segyuiscandata.cc
    uisegydef.cc
    uisegydefdlg.cc
    uisegydirectinserter.cc
    uisegyexamine.cc
    uisegyexp.cc
    uisegyimpdlg.cc
    uisegyimptype.cc
    uisegymanip.cc
    uisegypi.cc
    uisegyread.cc
    uisegyreaddlg.cc
    uisegyreadstarter.cc
    uisegyreadstartinfo.cc
    uisegyresortdlg.cc
    uisegyscandlg.cc
    uisegysip.cc
    uisegytool.cc
    uisegytrchdrvalplot.cc
    uisegywriteopts.cc
    uiwellimpsegyvsp.cc
)

SET( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )
SET( OD_MODULE_PROGS od_SEGYExaminer.cc )
OD_INIT_MODULE()
