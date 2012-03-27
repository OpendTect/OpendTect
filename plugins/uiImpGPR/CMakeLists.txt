#_______________________Pmake___________________________________________________
#
#	Makefile : 	uiImpGPR plugin
# 	Feb 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012-03-27 11:51:31 cvsnageswara Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiODMain)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	dztimporter.cc
	uiimpgprpi.cc)
OD_INIT_MODULE()
