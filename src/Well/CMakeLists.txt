#_______________________Pmake___________________________________________________
#
#	Makefile : 	Well
# 	Aug 2003	A.H. Bril
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012-01-23 20:55:23 cvskris Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS General)
SET(OD_MODULE_SOURCES
	well.cc wellman.cc welldisp.cc
	wellreader.cc wellwriter.cc welltransl.cc
	wellimpasc.cc wellextractdata.cc
	initwell.cc wellt2dtransform.cc
	     )
OD_INIT_MODULE()

