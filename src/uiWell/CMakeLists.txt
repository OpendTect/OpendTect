#_______________________Pmake___________________________________________________
#
#	Makefile : 	uiWell
# 	Jul 2000	A.H. Bril
#	RCS :		$Id: CMakeLists.txt,v 1.5 2012-03-23 09:26:36 cvsbruno Exp $
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
		uiwelllogcalcinpdata.cc
		uiwelllogdisplay.cc
		uiwelllogtools.cc
		uiwellman.cc
		uiwellmarkerdlg.cc
		uiwellpartserv.cc
		uiwellposprov.cc
		uiwellpropertyrefsel.cc
		uiwellrdmlinedlg.cc
		uiwellsel.cc
		uiwellstratdisplay.cc
		uiwellt2dconv.cc 
)
OD_INIT_MODULE()
