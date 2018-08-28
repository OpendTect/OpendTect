#_______________________Pmake___________________________________________________
#
#	Makefile : 	WellAttrib
# 	Jul 2000	Bruno Chaintreuil
#_______________________________________________________________________________


set(OD_MODULE_DEPS Well Attributes Strat )
set(OD_MODULE_SOURCES
		createattriblog.cc
		createlogcube.cc
		initwellattrib.cc
		stratseisevent.cc
		stratsynthdatamgr.cc
		stratsynthexp.cc
		stratsynthlevel.cc
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
