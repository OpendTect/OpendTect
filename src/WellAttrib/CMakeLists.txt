#_______________________Pmake___________________________________________________
#
#	Makefile : 	WellAttrib
# 	Jul 2000	Bruno Chaintreuil
#_______________________________________________________________________________

SET(OD_MODULE_DEPS Well Attributes Strat )
SET(OD_MODULE_SOURCES
		createattriblog.cc
		createlogcube.cc
		initwellattrib.cc
		stratseisevent.cc
		stratsynth.cc
		stratsynthexp.cc
		stratsynthgenparams.cc
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
OD_INIT_MODULE()
