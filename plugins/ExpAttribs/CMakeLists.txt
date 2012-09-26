#_______________________Pmake___________________________________________________
#
#	Makefile : 	ExpAttribs plugin
# 	Feb 2012	Nageswara
#	RCS :		$Id$
#_______________________________________________________________________________

SET(OD_MODULE_DEPS AttributeEngine)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	expattribspi.cc
	grubbsfilterattrib.cc
	semblanceattrib.cc)
SET( OD_PLUGIN_ALO_EXEC ${OD_ATTRIB_EXECS} )
OD_INIT_MODULE()
