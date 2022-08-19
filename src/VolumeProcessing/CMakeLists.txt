#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( OD_MODULE_DEPS Velocity Well )
set( OD_TEST_PROGS volprocchain.cc )
set( OD_MODULE_SOURCES
	initvolumeprocessing.cc
	velocitygridder.cc
	volprocattrib.cc
	volprocbodyfiller.cc
	volprocchain.cc
	volprocchainexec.cc
	volprocchainoutput.cc
	volprochorinterfiller.cc
	volproclateralsmoother.cc
	volprocprocessor.cc
	volprocsmoother.cc
	volprocstatscomputer.cc
	volprocstep.cc
	volprocsurfacelimitedfiller.cc
	volproctrans.cc
	volprocvolreader.cc
	voxelconnectivityfilter.cc
	wellloginterpolator.cc
)
set( OD_MODULE_BATCHPROGS od_process_volume.cc )

OD_INIT_MODULE()
