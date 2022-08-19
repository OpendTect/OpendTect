#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

SET(OD_MODULE_DEPS General)
SET(OD_MODULE_SOURCES
	initwell.cc
	directionalsurvey.cc
	laswriter.cc
	well.cc
	wellascio.cc
	welld2tmodel.cc
	welldatafilter.cc
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

SET(OD_MODULE_PROGS od_WellMan.cc )

OD_INIT_MODULE()
