#_______________________Pmake___________________________________________________
#
#	Makefile : 	MadagascarAttribs
# 	March 2012	Nageswara
#_______________________________________________________________________________

set(OD_MODULE_DEPS Attributes)
set(OD_IS_PLUGIN yes)
set(OD_MODULE_SOURCES
	madagcattrib.cc
	madattribspi.cc)
set( OD_PLUGIN_ALO_EXEC ${OD_ATTRIB_EXECS} )
OD_INIT_MODULE()
