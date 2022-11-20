#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( OD_MODULE_DEPS Well Attributes Strat )
set( OD_FOLDER "Base" )

set( OD_MODULE_SOURCES
	initwellattrib.cc
	createattriblog.cc
	createlogcube.cc
	stratseisevent.cc
	stratsynth.cc
	stratsynthexp.cc
	stratsynthgenparams.cc
	stratsynthlevel.cc
	syntheticdata.cc
	wellhorpos.cc
	welllogattrib.cc
	welltied2tmodelmanager.cc
	welltiedata.cc
	welltieextractdata.cc
	welltiegeocalculator.cc
	welltiepickset.cc
	welltiesetup.cc
	welltietoseismic.cc
)

set( OD_TEST_PROGS
	stratsynthdatamgr.cc
)

OD_INIT_MODULE()
