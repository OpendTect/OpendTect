#_______________________Pmake___________________________________________________
#
#	Makefile : 	DataPointSet demo
# 	March 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012-03-01 06:38:50 cvsnageswara Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiODMain)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	uidpsdemopi.cc
	uidpsdemo.cc)
OD_INIT_MODULE()
