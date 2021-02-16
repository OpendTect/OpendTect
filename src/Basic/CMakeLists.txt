#_______________________CMake___________________________________________________
#
#	Makefile : 	Basic
# 	July 1995	Bert
#_______________________________________________________________________________

set( OD_USEQT Core )
set( OD_USEBREAKPAD Yes )

if ( UNIX )
    option ( OD_ENABLE_MEMDEBUGGER_CODE
	    "Enable code that gives less false memory violation positives" OFF )
    if ( OD_ENABLE_MEMDEBUGGER_CODE )
	add_definitions("-DmMEM_DEBUGGER")
    endif()
endif( UNIX )

set( QT_MOC_HEADERS
	qtimercomm.h
	i_qfilesystemwatcher.h
)

set( OD_TEST_PROGS
	file.cc
	memcopying.cc
	od_iostream.cc
	oscommand.cc
	pythoncomm.cc
	svnversion.cc
	Basic_Composite.cc
)

if ( UNIX )
    list( APPEND OD_TEST_PROGS gason.cc )
endif()

set( OD_MODULE_SOURCES
	applicationdata.cc
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
	coordsystem.cc
	cubesampling.cc
	dateinfo.cc
	datapack.cc
	dbkey.cc
	debug.cc
	dirlist.cc
	enums.cc
	executor.cc
	factory.cc
	file.cc
	fileformat.cc
	filemonitor.cc
	filemultispec.cc
	filepath.cc
	filesystemaccess.cc
	fullhorsubsel.cc
	fullsubsel.cc
	gason.cpp
	genc.cc
	gpuprog.cc
	globexpr.cc
	iopar.cc
	initbasic.cc
	latlong.cc
	legal.cc
	linecolldataimpl.cc
	math2.cc
	moddepmgr.cc
	monitor.cc
	namedobj.cc
	objdisposer.cc
	od_iostream.cc
	oddirs.cc
	odimage.cc
	odinst.cc
	odjson.cc
	odmemory.cc
	odstring.cc
	oduicommon.cc
	odver.cc
	oscommand.cc
	plugins.cc
	posidxsubsel.cc
	posinfo.cc
	posinfo2d.cc
	posinfo2dsurv.cc
	position.cc
	progressmeter.cc
	pythonaccess.cc
	qstreambuf.cc
	refcount.cc
	repos.cc
	rowcol.cc
	safefileio.cc
	separstr.cc
	settings.cc
	settingsaccess.cc
	sighndl.cc
	string2.cc
	stringbuilder.cc
	strmoper.cc
	strmprov.cc
	surveydisklocation.cc
	survgeom.cc
	survgeommgr.cc
	survinfo.cc
	survsubsel.cc
	task.cc
	thread.cc
	threadwork.cc
	texttranslation.cc
	timefun.cc
	timer.cc
	trckeysampling.cc
	uistring.cc
	uistrings.cc
	uistringset.cc
	winutils.cc
	zdomain.cc
	zsubsel.cc
)

if ( ITTNOTIFY_FOUND )
    set ( OD_MODULE_DEPS ${ITTNOTIFY_LIBRARY} )
    list ( APPEND OD_MODULE_INCLUDESYSPATH ${ITTNOTIFY_INCLUDE_DIR} )
endif()

if ( NOT (CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR) )
    if ( UNIX )
	set ( FILEEXT sh )
    else()
	set ( FILEEXT cmd )
    endif()
    execute_process(
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
		    "${CMAKE_SOURCE_DIR}/bin/process_dumpfile.${FILEEXT}"
		    "${PROJECT_BINARY_DIR}/bin/process_dumpfile.${FILEEXT}" )
    execute_process(
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
		    "${CMAKE_SOURCE_DIR}/testscripts/script with space.${FILEEXT}"
		    "${PROJECT_BINARY_DIR}/testscripts/script with space.${FILEEXT}" )
    execute_process(
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CMAKE_SOURCE_DIR}/testscripts/count_to_1000.csh"
                    "${PROJECT_BINARY_DIR}/testscripts/count_to_1000.csh" )

    if ( UNIX )
	set ( FILEEXT sh )
    else()
	set ( FILEEXT bat )
    endif()
    execute_process(
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
		    "${CMAKE_SOURCE_DIR}/bin/od_external.${FILEEXT}"
		    "${PROJECT_BINARY_DIR}/bin/od_external.${FILEEXT}" )
endif()

OD_INIT_MODULE()
