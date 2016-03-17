#_______________________Pmake___________________________________________________
#
#	Makefile : 	Hello plugin
# 	Jan 2012	Nageswara
#_______________________________________________________________________________

set(OD_MODULE_DEPS Basic)
set(OD_IS_PLUGIN yes)
set(OD_MODULE_SOURCES
	hellopi.cc)
set( OD_NO_ALO_ENTRY 1 )
OD_INIT_MODULE()
