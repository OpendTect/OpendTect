#_______________________Pmake___________________________________________________
#
#	Makefile : 	Basic utilities
# 	July 1995	A.H. Bril
#	RCS :		$Id: CMakeLists.txt,v 1.3 2012-01-11 14:04:03 cvskris Exp $
#_______________________________________________________________________________

SET(LIB_NAME Basic)
SET(OD_USEQT Core)
SET(QT_MOC_HEADERS qfilesystemcomm.h qtimercomm.h)
SET(LIB_SOURCES genc.c filegen.c errnomsg.c oddirs.c
	    arrayndinfo.cc
	    arrayndslice.cc
	    ascstream.cc
	    bindatadesc.cc
	    bufstring.cc
	    callback.cc
	    color.cc
	    compoundkey.cc
	    conn.cc
	    cubesampling.cc
	    dateinfo.cc
	    datapack.cc
	    debug.cc
	    dirlist.cc
	    enums.cc
	    errh.cc
	    executor.cc
	    factory.cc
	    fontdata.cc
	    file.cc
	    filepath.cc
	    globexpr.cc
	    hostdata.cc
	    iopar.cc
	    latlong.cc
	    math2.cc
	    moddepmgr.cc
	    namedobj.cc
	    odimage.cc
	    odmemory.cc
	    odver.cc
	    plugins.cc
	    posinfo.cc
	    posinfo2d.cc
	    position.cc
	    positionlist.cc
	    progressmeter.cc
	    rcol2coord.cc
	    repos.cc
	    rowcol.cc
	    safefileio.cc
	    separstr.cc
	    settings.cc
	    sighndl.cc
	    staticsdesc.cc
	    staticstring.cc
	    string2.cc
	    strmoper.cc
	    strmprov.cc
	    survinfo.cc
	    surv2dgeom.cc
	    task.cc
	    thread.cc
	    threadwork.cc
	    timefun.cc
	    timer.cc
	    undefarray.cc
	    undo.cc
	    veldesc.cc
	    winutils.cc
	    zdomain.cc
	    ziputils.cc )
OD_INIT_MODULE()
