#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( OD_MODULE_DEPS Velocity )
set( OD_FOLDER "Base" )

set( OD_MODULE_SOURCES
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

set( OD_MODULE_BATCHPROGS
	od_process_prestack.cc
)

set( OD_BATCH_TEST_PROGS
	angle_computer.cc
	mute.cc
)

OD_INIT_MODULE()
