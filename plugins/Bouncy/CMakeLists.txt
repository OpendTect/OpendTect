#_______________________Pmake___________________________________________________
#
#	Makefile : 	Bouncy plugin
# 	Jan 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.4 2012/06/21 05:21:07 cvsraman Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiODMain)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	bouncycontroller.cc
	bouncypi.cc)
SET( OD_NO_ALO_ENTRY 1 )
OD_INIT_MODULE()
