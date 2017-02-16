#_______________________Pmake___________________________________________________
#
#	Makefile : 	General utilities
# 	July 1995	A.H. Bril
#_______________________________________________________________________________


set(OD_MODULE_DEPS Geometry MMProc )

set ( OD_TEST_PROGS seisbuf.cc seiskeytracker.cc seisprovider.cc )

set(OD_MODULE_SOURCES
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
	seisbayesclass.cc
	seisbuf.cc
	seiscbvs.cc
	seiscbvs2d.cc
	seiscbvsimpfromothersurv.cc
	seiscbvsps.cc
	seiscopy.cc
	seismscprov.cc
	seiscube2linedata.cc
	seisdatapack.cc
	seiseventsnapper.cc
	seisfetcher.cc
	seisimpbpsif.cc
	seisimporter.cc
	seisimpps.cc
	seisinfo.cc
	seisioobjinfo.cc
	seisiosimple.cc
	seisjobexecprov.cc
	seiskeytracker.cc
	seislineprovider.cc
	seisloader.cc
	seismerge.cc
	seismulticubeps.cc
	seisposindexer.cc
	seispreload.cc
	seisprovider.cc
	seisproviderset.cc
	seisps2dprovider.cc
	seisps3dprovider.cc
	seispsioprov.cc
	seispsmerge.cc
	seisrandlineto2d.cc
	seisread.cc
	seisreaderset.cc
	seisresampler.cc
	seisstatscalc.cc
	seisselection.cc
	seissingtrcproc.cc
	seisstatinfo.cc
	seisstor.cc
	seistrc.cc
	seistrcprop.cc
	seistrctr.cc
	seisvolprovider.cc
	seiswrite.cc
	seiszaxisstretcher.cc
	stratsynthgenparams.cc
	syntheticdata.cc
	synthseis.cc
	timedepthconv.cc
	wavelet.cc
	waveletio.cc
	waveletattrib.cc
	waveletextractor.cc
	waveletmanager.cc
)

set(OD_MODULE_BATCHPROGS
	    od_copy_seis.cc
	    od_process_2dto3d.cc
	    od_process_time2depth.cc
	    od_process_segyio.cc
)

OD_INIT_MODULE()
