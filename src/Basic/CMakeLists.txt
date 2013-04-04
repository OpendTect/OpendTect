#_______________________Pmake___________________________________________________
#
#	Makefile : 	Basic utilities
# 	July 1995	A.H. Bril
#	RCS :		$Id$
#_______________________________________________________________________________

set( OD_USEQT Core)
set( OD_USEBREAKPAD No)
set( OD_USEZLIB Yes)
set( QT_MOC_HEADERS qfilesystemcomm.h qtimercomm.h i_qptr.h)
set( OD_TEST_PROGS svnversion.cc
		sets.cc
		threads.cc
		commandlineparser.cc
		refcount.cc )
set( OD_MODULE_SOURCES genc.cc oddirs.cc
	    ailayer.cc
	    arrayndinfo.cc
	    arrayndslice.cc
	    ascstream.cc
	    bindatadesc.cc
	    bufstring.cc
	    callback.cc
	    color.cc
	    compoundkey.cc
	    commandlineparser.cc
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
	    qptr.cc
	    safefileio.cc
	    separstr.cc
	    settings.cc
	    sighndl.cc
	    staticsdesc.cc
	    staticstring.cc
	    string2.cc
	    strmoper.cc
	    strmprov.cc
	    survgeom.cc
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
OD_INIT_MODULE()
