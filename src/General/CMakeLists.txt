#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( OD_MODULE_DEPS Algo )
set( OD_FOLDER "Base" )
set( OD_USEQT Core ) # for the ziphandler.cc only
set( OD_USEZLIB TRUE )

set( OD_MODULE_SOURCES
	array2dbitmap.cc
	basemap.cc
	batchjobdispatch.cc
	bidvsetarrayadapter.cc
	bitmap2rgb.cc
	bitmapmgr.cc
	cbvsinfo.cc
	cbvsreader.cc
	cbvsreadmgr.cc
	cbvswritemgr.cc
	cbvswriter.cc
	coltabindex.cc
	coltabmapper.cc
	coltabsequence.cc
	conn.cc
	ctxtioobj.cc
	databuf.cc
	datainpspec.cc
	datainterp.cc
	datapackbase.cc
	datapointset.cc
	dpsdispmgr.cc
	draw.cc
	elasticpropsel.cc
	flatauxdataeditor.cc
	flatview.cc
	fontdata.cc
	generalinfo.cc
	geojson.cc
	geojsonwriter.cc
	geom2dascio.cc
	geometryio.cc
	giswriter.cc
	googlexmlwriter.cc
	grid2d.cc
	hdf5access.cc
	helpview.cc
	ibmformat.cc
	imagedeftr.cc
	iodir.cc
	iodirentry.cc
	ioman.cc
	ioobj.cc
	iostrm.cc
	keyboardevent.cc
	keyenum.cc
	linekey.cc
	linesetposinfo.cc
	mathexpression.cc
	mathformula.cc
	menuhandler.cc
	mnemonics.cc
	mousecursor.cc
	mouseevent.cc
	odiconfile.cc
	picklocation.cc
	pickretriever.cc
	pickset.cc
	pixmapdesc.cc
	posidxpairdataset.cc
	posidxpairvalset.cc
	posimpexppars.cc
	posinfodetector.cc
	posvecdataset.cc
	property.cc
	propertyref.cc
	rangeposprovider.cc
	raytracerrunner.cc
	reflcalcrunner.cc
	rockphysics.cc
	serverprogtool.cc
	simpletimedepthmodel.cc
	staticsdesc.cc
	stratlevel.cc
	surveyfile.cc
	survgeometrytransl.cc
	tableascio.cc
	tableconv.cc
	tablemodel.cc
	transl.cc
	unitofmeasure.cc
	usrinputobj.cc
	variogramcomputers.cc
	veldesc.cc
	zaxistransformutils.cc
	ziparchiveinfo.cc
	ziphandler.cc
	ziputils.cc
	initgeneral.cc
)

set( OD_MODULE_PROGS od_DBMan.cc )
set( OD_MODULE_TESTPROGS test_exit_program.cc )

if ( UNIX )
    set( OD_TEST_PROGS ibmformat.cc )
endif()

list( APPEND OD_TEST_PROGS
	mathexpression.cc
	mathformula.cc
	mnemonics.cc
	posidxpairvalset.cc
	raytracerrunner.cc
	reflectivitysampler.cc
	tempsurvey.cc
	unitofmeasure.cc
	veldesc.cc
)
set( OD_NIGHTLY_TEST_PROGS
	surveychanger.cc
	zip.cc
)

if ( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" )
    if ( CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.2 )
	message( STATUS "Turning down optimizations on ibmformat.cc to avoid "
		 "compiler problems." )
	set_source_files_properties( ibmformat.cc PROPERTIES COMPILE_FLAGS -O )
    endif()
endif()

OD_INIT_MODULE()

set( TEST_EXPECTEDRES "1" )
ADD_TEST_PROGRAM( "test_exit_program" )
