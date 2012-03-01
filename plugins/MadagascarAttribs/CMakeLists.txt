#_______________________Pmake___________________________________________________
#
#	Makefile : 	MadagascarAttribs
# 	March 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012-03-01 06:39:38 cvsnageswara Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS Attributes)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	madagcattrib.cc
	madattribspi.cc)
OD_INIT_MODULE()
