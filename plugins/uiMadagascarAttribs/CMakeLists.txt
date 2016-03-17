#_______________________Pmake___________________________________________________
#
#	Makefile : 	Madagascar Attributes, User Interface
# 	March 2012	Nageswara
#_______________________________________________________________________________

set(OD_MODULE_DEPS uiAttributes MadagascarAttribs)
set(OD_IS_PLUGIN yes)
set(OD_MODULE_SOURCES
	uimadagcattrib.cc
	uimadattribspi.cc)
set( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )
set( OD_NO_ALO_ENTRY 1 )
OD_INIT_MODULE()
