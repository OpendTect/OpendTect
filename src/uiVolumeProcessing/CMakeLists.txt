#_______________________Pmake___________________________________________________
#
#	Makefile : 	ui - Volume Processing
# 	Jul 2000	Kris
#_______________________________________________________________________________

set(OD_MODULE_DEPS uiVelocity uiWell uiAttributes uiEarthModel VolumeProcessing )
set(OD_MODULE_SOURCES
	inituivolumeprocessing.cc
	uivelocitygridder.cc
	uivolprocattrib.cc
	uivolprocchain.cc
	uivolprocbatchsetup.cc
	uivolprochorinterfiller.cc
	uivolproclateralsmoother.cc
	uivolprocbodyfiller.cc
	uivolprocpartserv.cc
	uivolprocregionfiller.cc
	uivolprocsmoother.cc
	uivolprocsurfacelimitedfiller.cc
	uivolprocudfreplacer.cc
	uivolprocvolreader.cc
	uiwellloginterpolator.cc
)

OD_INIT_MODULE()
