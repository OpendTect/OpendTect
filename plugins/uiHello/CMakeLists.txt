#_______________________Pmake___________________________________________________
#
#	Makefile : 	Hello World plugin
# 	March 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012-03-01 12:27:45 cvsnageswara Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiODMain)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	faultextractor.cc
	uifaultautoextractorpi.cc
	uitracksettingdlg.cc)
OD_INIT_MODULE()
