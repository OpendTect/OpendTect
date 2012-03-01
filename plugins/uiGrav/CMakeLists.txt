#_______________________Pmake___________________________________________________
#
#	Makefile : 	uiGrav plugin
# 	March 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012-03-01 11:58:15 cvsnageswara Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiODMain)
SET(OD_USECOIN 1)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	grav.cc
	gravhorcalc.cc
	uigravhorcalc.cc
	uigravpi.cc)
OD_INIT_MODULE()
