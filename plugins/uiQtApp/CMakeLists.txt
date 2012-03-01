#_______________________Pmake___________________________________________________
#
#	Makefile : 	Qt App plugin example
# 	March 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012-03-01 12:40:17 cvsnageswara Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiODMain)
SET(OD_IS_PLUGIN yes)
SET(OD_USEQT Gui)
SET(OD_MODULE_SOURCES
	qtclss.cc
	uiqtapppi.cc)
OD_INIT_MODULE()
