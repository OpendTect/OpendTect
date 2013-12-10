#_______________________Pmake___________________________________________________
#
#	Makefile : 	VMB::VelocityFunction
# 	2008		Kris
#	RCS :		$Id$
#_______________________________________________________________________________

set( OD_MODULE_DEPS Velocity)
set( OD_TEST_PROGS volprocchain.cc )
set( OD_MODULE_SOURCES
		initvolumeprocessing.cc
		velocitygridder.cc
		volprocchain.cc
		volprocattrib.cc
		volproclateralsmoother.cc
		volprocsmoother.cc
		volproctrans.cc
		volprocbodyfiller.cc
		volprocsurfacelimitedfiller.cc
		volprocvolreader.cc
)
set( OD_MODULE_BATCHPROGS od_process_volume.cc )
OD_INIT_MODULE()

