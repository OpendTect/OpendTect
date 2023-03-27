#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( OD_MODULE_DEPS uiIo uiSeis uiWellAttrib )
set( OD_FOLDER "OD Free Plugins" )
set( OD_IS_PLUGIN yes )
set( OD_MODULE_SOURCES
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
    uisegyreadstarter.cc
    uisegyreadstartinfo.cc
    uisegyresortdlg.cc
    uisegyscandlg.cc
    uisegysip.cc
    uisegysipclassic.cc
    uisegytrchdrvalplot.cc
    uisegywriteopts.cc
    uiwellimpsegyvsp.cc
    uisegytoolspi.cc
)

set( OD_MODULE_PROGS od_SEGYExaminer.cc )
set( OD_PLUGIN_ALO_EXEC ${OD_SURVEY_EXECS} ${DGB_SEGY_EXECS} )

OD_INIT_MODULE()
