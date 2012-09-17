#_______________________Pmake___________________________________________________
#
#	Makefile : 	ExpAttribs plugin
# 	Feb 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.5 2012/06/21 05:21:07 cvsraman Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS AttributeEngine)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	expattribspi.cc
	grubbsfilterattrib.cc
	semblanceattrib.cc)
SET( OD_PLUGIN_ALO_EXEC ${OD_ATTRIB_EXECS} )
SET( OD_NO_ALO_ENTRY 1 )
OD_INIT_MODULE()
