#_______________________Pmake___________________________________________________
#
#	Makefile : 	ui - Volume Processing
# 	Jul 2000	A.H. Bril
#	RCS :		$Id$
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiVelocity uiAttributes uiEarthModel VolumeProcessing)
SET(OD_MODULE_SOURCES
	inituivolumeprocessing.cc
	uivelocitygridder.cc
	uivolprocattrib.cc
	uivolprocchain.cc
	uivolprocbatchsetup.cc
	uivolproclateralsmoother.cc
	uivolprocbodyfiller.cc
	uivolprocpartserv.cc
	uivolprocsmoother.cc
	uivolprocsurfacelimitedfiller.cc
	uivolprocvolreader.cc
)

OD_INIT_MODULE()
