#_______________________Pmake___________________________________________________
#
#	Makefile : 	Experimental Attributes
# 	March 2012	Nageswara
#	RCS :		$Id$
#_______________________________________________________________________________

SET(OD_MODULE_DEPS ExpAttribs uiODMain)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	uiexpattribspi.cc
	uigrubbsfilterattrib.cc
	uisemblanceattrib.cc)
SET( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )
OD_INIT_MODULE()
