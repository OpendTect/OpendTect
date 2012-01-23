#_______________________Pmake___________________________________________________
#
#	Makefile : 	ui - Strat
# 	Jun 2007	Bert
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012-01-23 20:55:23 cvskris Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiIo Strat )
SET(OD_MODULE_SOURCES
	uistratreftree.cc uistrattreewin.cc uistratutildlgs.cc
	uistratlvlsel.cc uistratdisplay.cc uistratdispdata.cc
	uistratlvllist.cc uistratselunits.cc
	uistratlaymodtools.cc uistratlayseqgendesc.cc uistratlaymoddisp.cc
	uistratlayseqattrsetbuild.cc uilayseqattribed.cc
	inituistrat.cc
)

OD_INIT_MODULE()
