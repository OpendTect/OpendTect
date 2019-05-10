#_______________________Pmake___________________________________________________
#
#	Makefile : 	Well
# 	Aug 2003	Bert
#_______________________________________________________________________________


set(OD_MODULE_DEPS General)
set(OD_MODULE_SOURCES
	initwell.cc
	directionalsurvey.cc
	wellascio.cc
	well.cc
	welld2tmodel.cc
	welldahobj.cc
	welldisp.cc
	wellelasticmodelcomputer.cc
	wellextractdata.cc
	wellhdf5reader.cc
	wellhdf5writer.cc
	wellimplas.cc
	wellioprov.cc
	welllog.cc
	wellmanager.cc
	wellmarker.cc
	wellposprovider.cc
	wellreader.cc
	wellt2dtransform.cc
	welltrack.cc
	welltransl.cc
	wellwriter.cc
)

set ( OD_MODULE_PROGS od_WellMan.cc )

OD_INIT_MODULE()
