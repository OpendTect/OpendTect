#_______________________Pmake___________________________________________________
#
#	Makefile : 	Experimental Attributes
# 	March 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.4 2012/03/29 11:33:25 cvskris Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS ExpAttribs uiODMain)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	uiexpattribspi.cc
	uigrubbsfilterattrib.cc
	uisemblanceattrib.cc)
SET( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )
SET( OD_NO_ALO_ENTRY 1 )
OD_INIT_MODULE()
