#_______________________Pmake___________________________________________________
#
#	Makefile : 	Experimental Attributes
# 	March 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012-03-01 07:22:18 cvsnageswara Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS ExpAttribs uiODMain)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	uiexpattribspi.cc
	uigrubbsfilterattrib.cc
	uisemblanceattrib.cc)
OD_INIT_MODULE()
