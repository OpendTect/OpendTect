#_______________________Pmake___________________________________________________
#
#	Makefile : 	Bouncy plugin
# 	Jan 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012-02-29 05:32:22 cvsnageswara Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiODMain)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	bouncycontroller.cc
	bouncypi.cc)
OD_INIT_MODULE()
