#_______________________Pmake___________________________________________________
#
#	Makefile : 	uiWell
# 	Jul 2000	A.H. Bril
#	RCS :		$Id: CMakeLists.txt,v 1.2 2012-02-24 23:18:24 cvsnanne Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiStrat Well)
SET(OD_MODULE_SOURCES
		inituiwell.cc
		uid2tmodelgrp.cc
		uimultiwelllogsel.cc
		uisimplemultiwell.cc
		uiwelldahdisplay.cc
		uiwelldispprop.cc
		uiwelldisppropdlg.cc
		uiwelldisplay.cc
		uiwelldisplaycontrol.cc
		uiwelldisplaymarkeredit.cc
		uiwelldlgs.cc
		uiwellimpasc.cc
		uiwelllogcalc.cc
		uiwelllogdisplay.cc
		uiwelllogtools.cc
		uiwellman.cc
		uiwellmarkerdlg.cc
		uiwellpartserv.cc
		uiwellposprov.cc
		uiwellrdmlinedlg.cc
		uiwellstratdisplay.cc
		uiwellt2dconv.cc 
)
OD_INIT_MODULE()
