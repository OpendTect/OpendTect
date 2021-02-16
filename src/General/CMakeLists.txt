#_______________________Pmake___________________________________________________
#
#	Makefile : 	General utilities
# 	July 1995	Bert
#_______________________________________________________________________________

set ( OD_MODULE_DEPS Algo CRS )

list ( APPEND OD_TEST_PROGS General_Composite.cc )
set( OD_NIGHTLY_TEST_PROGS zip.cc )

list ( APPEND OD_MODULE_SOURCES
	array2dbitmap.cc
	autosaver.cc
	basemap.cc
	batchjobdispatch.cc
	beachballdata.cc
	bidvsetarrayadapter.cc
	bitmap2rgb.cc
	bitmapmgr.cc
	conn.cc
	cbvsinfo.cc cbvsreader.cc cbvsreadmgr.cc
	cbvswriter.cc cbvswritemgr.cc
	coltab.cc coltabmapper.cc coltabsequence.cc
	ctxtioobj.cc
	databuf.cc
	datainpspec.cc
	datainterp.cc
	datapackbase.cc
	datapointset.cc
	dbdir.cc
	dbman.cc
	displaypropertylinks.cc
	draw.cc
	elasticpropsel.cc
	flatauxdataeditor.cc
	flatview.cc
	fontdata.cc
	giswriter.cc
	generalinfo.cc
	geojson.cc
	geojsonwriter.cc
	googlexmlwriter.cc
	grid2d.cc
	hdf5access.cc
	helpview.cc
	ibmformat.cc
	initgeneral.cc
	ioobj.cc
	ioobjtags.cc
	iostrm.cc
	keyboardevent.cc
	keyenum.cc
	linesetposinfo.cc
	mathexpression.cc
	mathformula.cc
	menuhandler.cc
	mnemonics.cc
	mousecursor.cc
	mouseevent.cc
	odiconfile.cc
	odpresentationmgr.cc
	odviscommon.cc
	picklocation.cc
	pickretriever.cc
	pickset.cc
	picksetchangerecorder.cc
	picksetgroup.cc
	posimpexppars.cc
	posidxpairdataset.cc
	posidxpairvalset.cc
	posinfodetector.cc
	posvecdataset.cc
	probe.cc
	probetr.cc
	probemanager.cc
	property.cc
	propertyref.cc
	rangeposprovider.cc
	raytracerrunner.cc
	rockphysics.cc
	saveable.cc
	serverprogtool.cc
	stratlevel.cc
	survgeom2dascio.cc
	survgeometryio.cc
	survgeometrytransl.cc
	surveywalker.cc
	tableascio.cc
	tableconv.cc
	transl.cc
	unitofmeasure.cc
	userinputobj.cc
	variogramcomputers.cc
	volumedatapackzaxistransformer.cc
	zaxistransformutils.cc
	ziparchiveinfo.cc
	ziphandler.cc
	ziputils.cc
)

set (OD_USEQT Core) # for the ziphandler.cc only
set (OD_USEZLIB Yes)

if ( CMAKE_COMPILER_IS_GNUCC )
    if ( ${CMAKE_CXX_COMPILER} STREQUAL "/usr/bin/g++4" )
	message( "Turning down optimizations on ibmformat.cc to avoid "
		 "compiler problems." )
	set_source_files_properties( ibmformat.cc PROPERTIES COMPILE_FLAGS -O )
    endif()
endif ( CMAKE_COMPILER_IS_GNUCC )

set (OD_MODULE_PROGS od_DBMan.cc)
set (OD_MODULE_TESTPROGS test_exit_program.cc )
if ( WIN32 )
	set ( OD_ELEVATED_PERMISSIONS_GUI_PROGS od_Setup_Firewall.cc )
endif()

OD_INIT_MODULE()
