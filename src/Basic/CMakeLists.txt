#_______________________Pmake___________________________________________________
#
#	Makefile : 	Basic utilities
# 	July 1995	A.H. Bril
#	RCS :		$Id: CMakeLists.txt,v 1.12 2012-08-31 05:28:14 cvssalil Exp $
#_______________________________________________________________________________

SET(OD_USEQT Core)
SET(OD_USEZLIB Yes)
SET(QT_MOC_HEADERS qfilesystemcomm.h qtimercomm.h)
SET(OD_MODULE_SOURCES genc.c filegen.c oddirs.c
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
	    filesystemwatcher.cc
	    filepath.cc
	    globexpr.cc
	    hostdata.cc
	    iopar.cc
	    initbasic.cc
	    latlong.cc
	    math2.cc
	    moddepmgr.cc
	    namedobj.cc
	    odimage.cc
	    odinst.cc
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
	    winterminate.cc
	    zdomain.cc
	    ziputils.cc
	    ziparchiveinfo.cc 
	    ziphandler.cc )
SET(OD_MODULE_PROGS od_testzlib )
OD_INIT_MODULE()
