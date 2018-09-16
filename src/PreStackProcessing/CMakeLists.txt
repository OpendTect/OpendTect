#_______________________Pmake___________________________________________________
#
#	Makefile : 	Prestack stuff
# 	2008		K. Tingdahl
#_______________________________________________________________________________


set(OD_MODULE_DEPS Velocity)
set(OD_BATCH_TEST_PROGS
	angle_computer.cc
	mute.cc
	synthseis.cc
)
set(OD_MODULE_SOURCES
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
	prestacklateralstack.cc
	prestackmute.cc
	prestackmuteasciio.cc
	prestackmutedef.cc
	prestackmutedeftransl.cc
	prestackprocessor.cc
	prestackprocessortransl.cc
	prestackprop.cc
	prestackstacker.cc
	prestacksynthdataset.cc
	prestacktrimstatics.cc
	raysynthgenerator.cc
	semblancealgo.cc
)
set(OD_MODULE_BATCHPROGS od_process_prestack.cc )
OD_INIT_MODULE()
