#________________________________________________________________________
#
# Copyright:    dGB Beheer B.V.
# License:      https://dgbes.com/index.php/licensing
#________________________________________________________________________
#

SET(OD_MODULE_DEPS Well Attributes Strat )
SET(OD_MODULE_SOURCES
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
	welltiecshot.cc
	welltied2tmodelmanager.cc
	welltiedata.cc
	welltieextractdata.cc
	welltiegeocalculator.cc
	welltiepickset.cc
	welltiesetup.cc
	welltietoseismic.cc
	welltieunitfactors.cc
)

LIST( APPEND OD_TEST_PROGS
	stratsynthdatamgr.cc
)

OD_INIT_MODULE()
