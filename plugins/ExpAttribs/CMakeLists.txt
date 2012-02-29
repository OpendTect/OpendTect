#_______________________Pmake___________________________________________________
#
#	Makefile : 	ExpAttribs plugin
# 	Feb 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012-02-29 09:26:27 cvsnageswara Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS AttributeEngine)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	expattribspi.cc
	grubbsfilterattrib.cc
	semblanceattrib.cc)
OD_INIT_MODULE()
