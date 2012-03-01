#_______________________Pmake___________________________________________________
#
#	Makefile : 	Madagascar Attributes, User Interface
# 	March 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012-03-01 12:28:42 cvsnageswara Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiAttributes MadagascarAttribs)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	uimadagcattrib.cc
	uimadattribspi.cc)
OD_INIT_MODULE()
