#_______________________Pmake___________________________________________________
#
#	Makefile :	SEG-Y tools
#	Oct 2020	Wayne Mogg
#_______________________________________________________________________________

set(OD_MODULE_DEPS uiSeis uiWellAttrib)
set(OD_IS_PLUGIN yes)
set(OD_MODULE_SOURCES
    uisegytoolspi.cc
    uisegycommon.cc
    segyhdrkeydata.cc
    uisegyexamine.cc
    uisegytrchdrvalplot.cc
    uisegywriteopts.cc
    uisegydirectinserter.cc
    uisegydef.cc
    uisegyread.cc
    uisegyscandlg.cc
    uisegyimpdlg.cc
    uisegyreaddlg.cc
    uisegydefdlg.cc
    uisegymanip.cc
    segyuiscandata.cc
    segyvintageimporter.cc
    uisegyexp.cc
    uisegyfileselector.cc
    uisegyimptype.cc
    uisegyreadstarter.cc
    uisegyreadstartinfo.cc
    uisegyreadfinisher.cc
    uisegysipclassic.cc
    uisegyimpparsdlg.cc
    uisegymultivintageimporter.cc
    uisegyresortdlg.cc
    uisegysip.cc
    uiwellimpsegyvsp.cc
)

set( OD_PLUGIN_ALO_EXEC ${OD_SURVEY_EXECS} ${DGB_SEGY_EXECS} )
set( OD_MODULE_PROGS od_SEGYExaminer.cc )
OD_INIT_MODULE()
