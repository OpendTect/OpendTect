#_______________________Pmake___________________________________________________
#
#	Makefile : 	EventFreq plugin
# 	Jan 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012-02-29 09:16:35 cvsnageswara Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS Attributes)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	eventfreqattrib.cc
	eventfreqpi.cc)
OD_INIT_MODULE()
