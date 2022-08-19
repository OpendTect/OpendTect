#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

SET(OD_MODULE_DEPS Geometry MMProc )

set( OD_TEST_PROGS seisbuf.cc )
set( OD_BATCH_TEST_PROGS
	synthseis.cc
)

SET(OD_MODULE_SOURCES
	initseis.cc
	offsetazimuth.cc
	seisdatapackwriter.cc
	segydirect.cc
	segydirect2d.cc
	segydirecttr.cc
	segyfiledata.cc
	segyfiledef.cc
	segyhdr.cc
	segyhdrcalc.cc
	segyhdrdef.cc
	segyresorter.cc
	segyscanner.cc
	segytr.cc
	seis2ddata.cc
	seis2ddatafmtconverter.cc
	seis2dlineio.cc
	seis2dto3d.cc
	seis2dto3dinterpol.cc
	seisbayesclass.cc
	seisblocks.cc
	seisblocksreader.cc
	seisblockstr.cc
	seisbuf.cc
	seiscbvs.cc
	seiscbvs2d.cc
	seiscbvsimpfromothersurv.cc
	seiscbvsps.cc
	seiscopy.cc
	seiscubeprov.cc
	seiscube2linedata.cc
	seisdatapack.cc
	seisdatapackfromdps.cc
	seisdatapackzaxistransformer.cc
	seiseventsnapper.cc
	seisimpbpsif.cc
	seisimporter.cc
	seisimpps.cc
	seisinfo.cc
	seisioobjinfo.cc
	seisiosimple.cc
	seisjobexecprov.cc
	seismerge.cc
	seismulticubeps.cc
	seisparallelreader.cc
	seisposindexer.cc
	seisposprovider.cc
	seispreload.cc
	seispsioprov.cc
	seispsmerge.cc
	seisrandlineto2d.cc
	seisrandomprovider.cc
	seisrawtrcsseq.cc
	seisread.cc
	seisreaderset.cc
	seisresampler.cc
	seisscanner.cc
	seisselection.cc
	seissingtrcproc.cc
	seisstatscollector.cc
	seisstor.cc
	seistrc.cc
	seistrcprop.cc
	seistrctr.cc
	seiswrite.cc
	seiszaxisstretcher.cc
	synthseis.cc
	timedepthconv.cc
	wavelet.cc
	waveletattrib.cc
	waveletextractor.cc
)

SET(OD_MODULE_BATCHPROGS
	od_copy_seis.cc
	od_process_2dto3d.cc
	od_process_time2depth.cc
	od_process_segyio.cc
)

OD_INIT_MODULE()
