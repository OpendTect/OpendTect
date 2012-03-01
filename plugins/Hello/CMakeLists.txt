#_______________________Pmake___________________________________________________
#
#	Makefile : 	Hello plugin
# 	Jan 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012-03-01 04:26:46 cvsnageswara Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS Basic)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	hellopi.cc)
OD_INIT_MODULE()
