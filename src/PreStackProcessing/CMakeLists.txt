#________________________________________________________________________
#
# Copyright:    dGB Beheer B.V.
# License:      https://dgbes.com/index.php/licensing
#________________________________________________________________________
#

SET(OD_MODULE_DEPS Velocity)
SET(OD_BATCH_TEST_PROGS
	angle_computer.cc
	mute.cc
)
SET(OD_MODULE_SOURCES
	initprestackprocessing.cc
	prestackagc.cc
	prestackanglecomputer.cc
	prestackanglemute.cc
	prestackanglemutecomputer.cc
	prestackeventascio.cc
	prestackeventio.cc
	prestackevents.cc
	prestackeventsapi.cc
	prestackeventtracker.cc
	prestackeventtransl.cc
	prestackgather.cc
	prestacklateralstack.cc
	prestackmute.cc
	prestackmuteasciio.cc
	prestackmutedef.cc
	prestackmutedeftransl.cc
	prestackprocessor.cc
	prestackprocessortransl.cc
	prestackprop.cc
	prestackstacker.cc
	prestacktrimstatics.cc
	semblancealgo.cc
)
SET(OD_MODULE_BATCHPROGS od_process_prestack.cc )

OD_INIT_MODULE()
