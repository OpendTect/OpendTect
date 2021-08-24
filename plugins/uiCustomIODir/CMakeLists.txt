#_______________________Pmake___________________________________________________
#
#	Makefile : 	Create your own custom data directory
# 	Feb 2014:	Bert
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiODMain)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES uicustomiodirpi.cc)
SET( OD_NO_ALO_ENTRY 1 )
OD_INIT_MODULE()
