#_______________________Pmake___________________________________________________
#
#	Makefile : 	VMB::VelocityFunction
# 	2008		Kris
#	RCS :		$Id$
#_______________________________________________________________________________


add_definitions( -DmNoAutoUiString )

set( OD_MODULE_DEPS Velocity Well )
set( OD_TEST_PROGS volprocchain.cc )
set( OD_MODULE_SOURCES
	initvolumeprocessing.cc
	velocitygridder.cc
	volprocchain.cc
	volprocattrib.cc
	volprochorinterfiller.cc
	volproclateralsmoother.cc
	volprocsmoother.cc
	volprocstatscomputer.cc
	volproctrans.cc
	volprocbodyfiller.cc
	volprocsurfacelimitedfiller.cc
	volprocvolreader.cc
	wellloginterpolator.cc
)
set( OD_MODULE_BATCHPROGS od_process_volume.cc )
OD_INIT_MODULE()
