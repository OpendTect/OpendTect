#_______________________Pmake___________________________________________________
#
#	Makefile : 	ui - Strat
# 	Jun 2007	Bert
#	RCS :		$Id$
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiIo Strat )
SET(OD_MODULE_SOURCES
	uistratreftree.cc uistrattreewin.cc uistratutildlgs.cc
	uistratdisplay.cc uistratdispdata.cc
	uistratlvllist.cc uistratselunits.cc
	uistrateditlayer.cc
	uistratlaymodtools.cc uistratlayseqgendesc.cc uistratlaymoddisp.cc
	uistratlayseqattrsetbuild.cc uilayseqattribed.cc
	inituistrat.cc
)

OD_INIT_MODULE()
