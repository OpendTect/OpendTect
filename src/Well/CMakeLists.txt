#_______________________Pmake___________________________________________________
#
#	Makefile : 	Well
# 	Aug 2003	A.H. Bril
#	RCS :		$Id$
#_______________________________________________________________________________

SET(OD_MODULE_DEPS General)
SET(OD_MODULE_SOURCES
	well.cc welltrack.cc welld2tmodel.cc wellmarker.cc welllog.cc
	wellman.cc wellreader.cc wellwriter.cc welltransl.cc
	wellimpasc.cc wellextractdata.cc wellposprovider.cc
	initwell.cc wellt2dtransform.cc welldisp.cc
	     )
OD_INIT_MODULE()

