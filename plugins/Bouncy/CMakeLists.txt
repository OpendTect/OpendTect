#_______________________Pmake___________________________________________________
#
#	Makefile : 	Bouncy plugin
# 	Jan 2012	Nageswara
#	RCS :		$Id$
#_______________________________________________________________________________

set( OD_MODULE_DEPS uiODMain)
set( OD_IS_PLUGIN yes)
set( OD_MODULE_SOURCES
	bouncycontroller.cc
	bouncypi.cc)
set( OD_NO_ALO_ENTRY 1 )
OD_INIT_MODULE()

