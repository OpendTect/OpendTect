#_______________________Pmake___________________________________________________
#
#	Makefile : 	uiWell
# 	Jul 2000	A.H. Bril
#_______________________________________________________________________________

set(OD_MODULE_DEPS uiStrat Well)
set(OD_MODULE_SOURCES
	inituiwell.cc
	uibulkwellimp.cc
	uid2tmodelgrp.cc
	uilassip.cc
	uilaswriter.cc
	uimultiwelllogsel.cc
	uisimplemultiwell.cc
	uiwelldahdisplay.cc
	uiwelldispprop.cc
	uiwelldisppropdlg.cc
	uiwelldisplay.cc
	uiwelldisplayserver.cc
	uiwelldisplaycontrol.cc
	uiwelldisplaymarkeredit.cc
	uiwelldlgs.cc
	uiwellimpasc.cc
	uiwellinserter.cc
	uiwelllogcalc.cc
	uiwelllogimpexp.cc
	uiwelllogdisplay.cc
	uiwelllogtools.cc
	uiwellman.cc
	uiwellmarkersel.cc
	uiwellmarkerdlg.cc
	uiwellpartserv.cc
	uiwellposprov.cc
	uiwellpropertyrefsel.cc
	uiwellrdmlinedlg.cc
	uiwellsel.cc
	uiwellstratdisplay.cc
	uiwellt2dconv.cc
	uiwellwriteopts.cc
)
OD_INIT_MODULE()
