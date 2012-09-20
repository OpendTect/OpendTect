#_______________________Pmake___________________________________________________
#
#	Makefile : 	Madagascar Attributes, User Interface
# 	March 2012	Nageswara
#	RCS :		$Id$
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiAttributes MadagascarAttribs)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	uimadagcattrib.cc
	uimadattribspi.cc)
SET( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )
OD_INIT_MODULE()
