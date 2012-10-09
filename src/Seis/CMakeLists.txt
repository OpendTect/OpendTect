#_______________________Pmake___________________________________________________
#
#	Makefile : 	General utilities
# 	July 1995	A.H. Bril
#	RCS :		$Id$
#_______________________________________________________________________________

SET(OD_MODULE_DEPS Geometry MMProc )
SET(OD_MODULE_SOURCES
	    initseis.cc
	    offsetazimuth.cc
	    segydirect.cc
	    segydirecttr.cc
	    segyfiledata.cc
	    segyfiledef.cc
	    segyhdr.cc
	    segyhdrcalc.cc
	    segyhdrdef.cc
	    segyresorter.cc
	    segyscanner.cc
	    segytr.cc
	    seis2dline.cc
	    seis2dlineio.cc
	    seis2dto3d.cc
	    seisbayesclass.cc
	    seisbuf.cc
	    seiscbvs.cc
	    seiscbvs2d.cc
	    seiscbvsps.cc
	    seiscubeprov.cc
	    seiscube2linedata.cc
	    seiseventsnapper.cc
	    seisgeometryfrom2dlines.cc
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
	    seispreload.cc
	    seispsioprov.cc
	    seispsmerge.cc
	    seisrandlineto2d.cc
	    seisread.cc
	    seisresampler.cc
	    seisscanner.cc
	    seisselection.cc
	    seissingtrcproc.cc
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
SET(OD_MODULE_BATCHPROGS od_process_time2depth od_process_segyio )
OD_INIT_MODULE()

