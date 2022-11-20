#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( OD_MODULE_DEPS uiVelocity uiWell uiAttributes uiEarthModel VolumeProcessing )
set( OD_FOLDER "Base (GUI)" )

set( OD_MODULE_SOURCES
	inituivolumeprocessing.cc
	uivelocitygridder.cc
	uivolprocattrib.cc
	uivolprocbatchsetup.cc
	uivolprocbodyfiller.cc
	uivolprocchain.cc
	uivolprochorinterfiller.cc
	uivolproclateralsmoother.cc
	uivolprocpartserv.cc
	uivolprocsmoother.cc
	uivolprocsurfacelimitedfiller.cc
	uivolprocvolreader.cc
	uivoxelconnectivityfilter.cc
	uiwellloginterpolator.cc
)

OD_INIT_MODULE()
