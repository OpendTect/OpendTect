#_______________________Pmake___________________________________________________
#
#	Makefile : 	Create your own custom data directory
# 	Feb 2014:	Bert
#	RCS :		$Id$
#_______________________________________________________________________________

set(OD_MODULE_DEPS uiODMain)
set(OD_IS_PLUGIN yes)
set(OD_MODULE_SOURCES uicustomiodirpi.cc)
set( OD_NO_ALO_ENTRY 1 )
OD_INIT_MODULE()

