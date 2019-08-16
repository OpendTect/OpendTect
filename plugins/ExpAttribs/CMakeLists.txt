#_______________________Pmake___________________________________________________
#
#	Makefile : 	ExpAttribs plugin
# 	Feb 2012	Nageswara
#_______________________________________________________________________________

set(OD_MODULE_DEPS AttributeEngine)
set(OD_IS_PLUGIN yes)
set(OD_MODULE_SOURCES
	expattribspi.cc
	eventfreqattrib.cc
	grubbsfilterattrib.cc
	integratedtrace.cc
	corrmultiattrib.cc
	similaritybyaw.cc
)
set( OD_PLUGIN_ALO_EXEC ${OD_ATTRIB_EXECS} )
OD_INIT_MODULE()
