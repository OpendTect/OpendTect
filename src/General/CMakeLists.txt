#_______________________Pmake___________________________________________________
#
#	Makefile : 	General utilities
# 	July 1995	A.H. Bril
#_______________________________________________________________________________

add_custom_target( od_helpid ALL 
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${CMAKE_SOURCE_DIR}/external/doc_csh/od_helpids_orig.h
                    ${PROJECT_BINARY_DIR}/include/General/od_helpids.h
		COMMENT "Checking od_helpids.h" )

set ( OD_MODULE_DEPS Algo )
if ( NOT DEFINED WIN32 )
    set ( OD_TEST_PROGS ibmformat.cc )
endif()
list ( APPEND OD_TEST_PROGS mathexpression.cc mathformula.cc posidxpairdataset.cc posidxpairvalset.cc pickset.cc )
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
	coltabindex.cc coltabmapper.cc coltabsequence.cc
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
	geom2dascio.cc
	geometryio.cc
	googlexmlwriter.cc
	grid2d.cc
	helpview.cc
	ibmformat.cc
	initgeneral.cc
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
	mousecursor.cc
	mouseevent.cc
	odiconfile.cc
	odpresentationmgr.cc
	picklocation.cc
	pickretriever.cc
	pickset.cc
	picksetchangerecorder.cc
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
	saveable.cc
	stratlevel.cc
	survgeometrytransl.cc
	surveysectionprinfo.cc
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

if ( CMAKE_COMPILER_IS_GNUCC )
    if ( ${CMAKE_CXX_COMPILER} STREQUAL "/usr/bin/g++4" )
	message( "Turning down optimizations on ibmformat.cc to avoid "
		 "compiler problems." )
	set_source_files_properties( ibmformat.cc PROPERTIES COMPILE_FLAGS -O )
    endif()
endif ( CMAKE_COMPILER_IS_GNUCC )

OD_INIT_MODULE()
