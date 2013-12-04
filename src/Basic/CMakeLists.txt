#_______________________Pmake___________________________________________________
#
#	Makefile : 	Basic utilities
# 	July 1995	A.H. Bril
#	RCS :		$Id$
#_______________________________________________________________________________

set( OD_USEQT Core)
set( OD_USEBREAKPAD Yes)
set( OD_USEZLIB Yes)
set( QT_MOC_HEADERS qfilesystemcomm.h qtimercomm.h i_qptr.h)
set( OD_NIGHTLY_TEST_PROGS zip.cc )
set( OD_TEST_PROGS
		basic.cc
		callback.cc
		commandlineparser.cc
		cubesampling.cc
		file.cc
		math2.cc
		od_iostream.cc
		oscommand.cc
		refcount.cc
		sets.cc
		string.cc
		svnversion.cc
		threads.cc
		threadwork.cc
)
set( OD_MODULE_SOURCES
	    arrayndinfo.cc
	    arrayndslice.cc
	    ascstream.cc
	    ascbinstream.cc
	    bindatadesc.cc
	    bufstring.cc
	    callback.cc
	    color.cc
	    compoundkey.cc
	    commandlineparser.cc
	    cubesampling.cc
	    dateinfo.cc
	    datapack.cc
	    debug.cc
	    dirlist.cc
	    enums.cc
	    executor.cc
	    factory.cc
	    fontdata.cc
	    file.cc
	    filesystemwatcher.cc
	    filepath.cc
	    genc.cc
	    globexpr.cc
	    hostdata.cc
	    iopar.cc
	    initbasic.cc
	    latlong.cc
	    math2.cc
	    moddepmgr.cc
	    namedobj.cc
	    od_iostream.cc
	    oddirs.cc
	    odimage.cc
	    odinst.cc
	    odmemory.cc
	    odver.cc
	    oscommand.cc
	    plugins.cc
	    posinfo.cc
	    posinfo2d.cc
	    posinfo2dsurv.cc
	    position.cc
	    positionlist.cc
	    progressmeter.cc
	    repos.cc
	    rowcol.cc
	    qptr.cc
	    safefileio.cc
	    separstr.cc
	    settings.cc
	    sighndl.cc
	    staticstring.cc
	    string2.cc
	    strmoper.cc
	    strmprov.cc
	    survgeom.cc
	    survinfo.cc
	    task.cc
	    thread.cc
	    threadwork.cc
	    timefun.cc
	    timer.cc
	    undefarray.cc
	    undo.cc
	    winutils.cc
	    zdomain.cc
	    ziputils.cc
	    ziparchiveinfo.cc 
	    ziphandler.cc )

set ( OD_MODULE_PROGS test_exit_program.cc )

if ( ITTNOTIFY_FOUND )
    set ( OD_MODULE_STATIC_LIBS ${ITTNOTIFY_LIBRARY} )
    list ( APPEND OD_MODULE_INCLUDESYSPATH ${ITTNOTIFY_INCLUDE_DIR} )
endif()

OD_INIT_MODULE()
