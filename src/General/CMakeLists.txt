#_______________________Pmake___________________________________________________
#
#	Makefile : 	General utilities
# 	July 1995	A.H. Bril
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012-01-11 11:43:19 cvskris Exp $
#_______________________________________________________________________________

SET(LIB_NAME General)
SET(DEPS Algo )
SET(LIB_SOURCES arrayndutils.cc
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
	    price.cc
	    rangeposprovider.cc
	    raytracerrunner.cc
	    stratlevel.cc
	    tableascio.cc
	    tableconv.cc
	    transl.cc
	    unitofmeasure.cc
	    zaxistransformdatapack.cc )
OD_INIT_MODULE()


