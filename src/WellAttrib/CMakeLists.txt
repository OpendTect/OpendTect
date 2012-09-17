#_______________________Pmake___________________________________________________
#
#	Makefile : 	WellAttrib
# 	Jul 2000	Bruno Chaintreuil
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012/01/23 20:55:23 cvskris Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS Well AttributeEngine PreStackProcessing Strat )
SET(OD_MODULE_SOURCES
		createattriblog.cc
		createlogcube.cc
		stratseisevent.cc
		stratsynth.cc
		wellhorpos.cc
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

