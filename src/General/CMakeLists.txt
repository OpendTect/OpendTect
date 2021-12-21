#_______________________Pmake___________________________________________________
#
#	Makefile : 	General utilities
# 	July 1995	A.H. Bril
#_______________________________________________________________________________

set ( OD_MODULE_DEPS Algo CRS )
if ( UNIX )
    set ( OD_TEST_PROGS ibmformat.cc )
endif()

list ( APPEND OD_TEST_PROGS
	mathexpression.cc
	mathformula.cc
	mnemonics.cc
	posidxpairvalset.cc
	raytracerrunner.cc
)
set( OD_NIGHTLY_TEST_PROGS zip.cc )

list ( APPEND OD_MODULE_SOURCES
	array2dbitmap.cc
	basemap.cc
	batchjobdispatch.cc
	bidvsetarrayadapter.cc
	bitmap2rgb.cc
	bitmapmgr.cc
	conn.cc
	cbvsinfo.cc cbvsreader.cc cbvsreadmgr.cc
	cbvswriter.cc cbvswritemgr.cc
	coltabindex.cc coltabmapper.cc coltabsequence.cc
	ctxtioobj.cc
	databuf.cc
	datainpspec.cc
	datainterp.cc
	datapackbase.cc
	datapointset.cc
	displaypropertylinks.cc
	draw.cc
	elasticpropsel.cc
	flatauxdataeditor.cc
	flatview.cc
	fontdata.cc
	generalinfo.cc
	giswriter.cc
	geojson.cc
	geojsonwriter.cc
	googlexmlwriter.cc
	geom2dascio.cc
	geometryio.cc
	grid2d.cc
	hdf5access.cc
	helpview.cc
	ibmformat.cc
	initgeneral.cc
	iodir.cc
	iodirentry.cc
	ioman.cc
	ioobj.cc
	ioobjtags.cc
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
	posimpexppars.cc
	posidxpairdataset.cc
	posidxpairvalset.cc
	posinfodetector.cc
	posvecdataset.cc
	property.cc
	propertyref.cc
	rangeposprovider.cc
	raytracerrunner.cc
	rockphysics.cc
	serverprogtool.cc
	simpletimedepthmodel.cc
	stratlevel.cc
	surveyfile.cc
	survgeometrytransl.cc
	tableascio.cc
	tableconv.cc
	transl.cc
	unitofmeasure.cc
	usrinputobj.cc
	variogramcomputers.cc
	zaxistransformutils.cc
	ziparchiveinfo.cc
	ziphandler.cc
	ziputils.cc
)

set (OD_USEQT Core) # for the ziphandler.cc only
set (OD_USEZLIB Yes)
set (OD_MODULE_PROGS od_DBMan.cc)
set (OD_MODULE_TESTPROGS test_exit_program.cc )

if ( CMAKE_COMPILER_IS_GNUCC )
    if ( ${CMAKE_CXX_COMPILER} STREQUAL "/usr/bin/g++4" )
	message( "Turning down optimizations on ibmformat.cc to avoid "
		 "compiler problems." )
	set_source_files_properties( ibmformat.cc PROPERTIES COMPILE_FLAGS -O )
    endif()
endif ( CMAKE_COMPILER_IS_GNUCC )

OD_INIT_MODULE()
