#_______________________Pmake___________________________________________________
#
#	Makefile : 	General utilities
# 	July 1995	A.H. Bril
#	RCS :		$Id: CMakeLists.txt,v 1.8 2012-07-16 06:10:33 cvskris Exp $
#_______________________________________________________________________________

SET ( OD_MODULE_DEPS Algo )
SET ( OD_MODULE_SOURCES arrayndutils.cc
	    array2dbitmap.cc
	    basemap.cc
	    beachballdata.cc
	    bidvsetarrayadapter.cc
	    binidvalset.cc
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
	    geom2dascio.cc
	    googlexmlwriter.cc
	    grid2d.cc
	    helpview.cc
	    ibmformat.cc
	    initgeneral.cc
	    iodir.cc
	    iodirentry.cc
	    ioman.cc
	    ioobj.cc
	    iostrm.cc
	    iox.cc
	    keyboardevent.cc
	    keyenum.cc
	    linekey.cc
	    linesetposinfo.cc
	    mathexpression.cc
	    menuhandler.cc
	    mousecursor.cc
	    mouseevent.cc
	    pickretriever.cc
	    pickset.cc
	    posimpexppars.cc
	    posinfodetector.cc
	    posvecdataset.cc
	    property.cc
	    propertyref.cc
	    price.cc
	    rangeposprovider.cc
	    raytracerrunner.cc
	    rockphysics.cc
	    stratlevel.cc
	    tableascio.cc
	    tableconv.cc
	    transl.cc
	    unitofmeasure.cc
	    variogramcomputers.cc
	    zaxistransformdatapack.cc )

if ( CMAKE_COMPILER_IS_GNUCC )
    if ( NOT OD_DEBUG )
	if ( CMAKE_CXX_COMPILER MATCHES "/usr/bin/g++4" )
	    message( "Turning down optimizations on ibmformat.cc to avoid "
		     "compiler problems" )
	    set_source_files_properties( ibmformat.cc PROPERTIES COMPILE_FLAGS -O )
	endif()
    endif ( NOT OD_DEBUG )
endif ( CMAKE_COMPILER_IS_GNUCC )

OD_INIT_MODULE()


