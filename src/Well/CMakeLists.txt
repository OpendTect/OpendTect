#_______________________Pmake___________________________________________________
#
#	Makefile : 	Well
# 	Aug 2003	A.H. Bril
#	RCS :		$Id$
#_______________________________________________________________________________

SET(OD_MODULE_DEPS General)
SET(OD_MODULE_SOURCES
	initwell.cc
	wellascio.cc
	well.cc
	welld2tmodel.cc
	welldisp.cc
	wellelasticmodelcomputer.cc
	wellextractdata.cc
	wellimplas.cc
	wellioprov.cc
	welllog.cc
	wellman.cc
	wellmarker.cc
	wellposprovider.cc
	wellreader.cc
	wellt2dtransform.cc
	welltrack.cc
	welltransl.cc
	wellwriter.cc
)
OD_INIT_MODULE()

