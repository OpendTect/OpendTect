#_______________________Pmake___________________________________________________
#
#	Makefile : 	Hello World plugin
# 	March 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.2 2012-03-22 14:24:03 cvsbert Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiODMain)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	faultextractor.cc
	uifaultautoextractorpi.cc
	uitracksettingdlg.cc)
SET( OD_NO_ALO_ENTRY 1 )
OD_INIT_MODULE()
