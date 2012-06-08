#_______________________Pmake___________________________________________________
#
#	Makefile : 	Hello plugin
# 	Jan 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.6 2012-06-08 11:38:46 cvsraman Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS Basic)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	hellopi.cc)
SET( OD_NO_ALO_ENTRY 1 )
OD_INIT_MODULE()
