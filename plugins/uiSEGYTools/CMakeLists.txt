#_______________________Pmake___________________________________________________
#
#	Makefile :	SEG-Y tools
#	Jan 2021	Wayne Mogg
#_______________________________________________________________________________

set(OD_MODULE_DEPS uiSeis uiWellAttrib)
set(OD_IS_PLUGIN yes)
set(OD_MODULE_SOURCES
    uisegytoolspi.cc
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
    uisegymultilinesel.cc
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

set( OD_PLUGIN_ALO_EXEC ${OD_SURVEY_EXECS} ${DGB_SEGY_EXECS} )
set( OD_MODULE_PROGS od_SEGYExaminer.cc )
OD_INIT_MODULE()
