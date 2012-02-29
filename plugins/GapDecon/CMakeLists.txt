#_______________________Pmake___________________________________________________
#
#	Makefile : 	GapDecon plugin
# 	Feb 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012-02-29 10:59:48 cvsnageswara Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS Attributes)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	gapdeconattrib.cc
	gapdeconpi.cc)
OD_INIT_MODULE()
