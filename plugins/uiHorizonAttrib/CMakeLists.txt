#_______________________Pmake___________________________________________________
#
#	Makefile : 	Horizon attributes and related
# 	March 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012-03-07 10:44:29 cvsnageswara Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiODMain HorizonAttrib)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	uihorizonattrib.cc
	uihorattribpi.cc
	uistratamp.cc
	uiflattenedcube.cc
	uiisopachmaker.cc
	uicontourtreeitem.cc)
OD_INIT_MODULE()
