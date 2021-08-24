#_______________________Pmake___________________________________________________
#
#	Makefile : 	ui - Volume Processing
# 	Jul 2000	A.H. Bril
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
	uivolprocsmoother.cc
	uivolprocsurfacelimitedfiller.cc
	uivolprocvolreader.cc
	uiwellloginterpolator.cc
)

OD_INIT_MODULE()
