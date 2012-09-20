#_______________________Pmake___________________________________________________
#
#	Makefile : 	Hello World plugin
# 	March 2012	Nageswara
#	RCS :		$Id$
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiODMain)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	uihellopi.cc)
SET( OD_NO_ALO_ENTRY 1 )
OD_INIT_MODULE()
