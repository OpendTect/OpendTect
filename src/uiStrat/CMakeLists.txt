#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( OD_MODULE_DEPS uiIo Strat )
set( OD_FOLDER "Base (GUI)" )

set( OD_MODULE_SOURCES
	inituistrat.cc
	uistratdisplay.cc
	uistrateditlayer.cc
	uistratlaymodtools.cc
	uistratlayseqattrsetbuild.cc
	uistratlvllist.cc
	uistratreftree.cc
	uilayseqattribed.cc
	uistratdispdata.cc
	uistratlaymoddisp.cc
	uistratlayseqgendesc.cc
	uistratselunits.cc
	uistrattreewin.cc
	uistratutildlgs.cc
)

OD_INIT_MODULE()
