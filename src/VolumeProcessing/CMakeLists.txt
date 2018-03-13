#_______________________Pmake___________________________________________________
#
#	Makefile : 	VMB::VelocityFunction
# 	2008		Kris
#	RCS :		$Id: CMakeLists.txt 36818 2014-10-07 20:14:44Z nanne.hemstra@dgbes.com $
#_______________________________________________________________________________


set( OD_MODULE_DEPS Velocity Well )
set( OD_TEST_PROGS volprocchain.cc )
set( OD_MODULE_SOURCES
	initvolumeprocessing.cc
	velocitygridder.cc
	volprocattrib.cc
	volprocbodyfiller.cc
	volprocchain.cc
	volprocchainexec.cc
	volprochilbert.cc
	volprocchainoutput.cc
	volprochorinterfiller.cc
	volproclateralsmoother.cc
	volprocprocessor.cc
	volprocregionfiller.cc
	volprocsmoother.cc
	volprocstatscomputer.cc
	volprocstep.cc
	volprocsurfacelimitedfiller.cc
	volproctrans.cc
	volprocudfreplacer.cc
	volprocvolreader.cc
	wellloginterpolator.cc
)
set( OD_MODULE_BATCHPROGS od_process_volume.cc )
OD_INIT_MODULE()
